
#ifndef MYCOIN_BLOCKCHAIN_H
#define MYCOIN_BLOCKCHAIN_H

#include "httplib.h"
#include "Windows.h"
#include "json.h"
#include "sha256.h"
#include "urlparse.h"

#include <vector>
#include <string>
#include <set>
#include <iostream>


using nlohmann::json;


struct Transaction {
    std::string data;
};

void to_json(json &j, const Transaction &t) {
    j = json{{"data", t.data}};
}

void from_json(const json &j, Transaction &t) {
    j.at("data").get_to(t.data);
}

struct Block {
    size_t index;
    time_t timestamp;
    std::vector<Transaction> transactions;
    size_t nonce;
    std::string previous_hash;
};

void to_json(json &j, const Block &block) {
    j = json{{"index",         block.index},
             {"timestamp",     block.timestamp},
             {"transactions",  block.transactions},
             {"nonce",         block.nonce},
             {"previous_hash", block.previous_hash}
    };
}

void from_json(const json &j, Block &block) {
    j.at("index").get_to(block.index);
    j.at("timestamp").get_to(block.timestamp);
    j.at("transactions").get_to(block.transactions);
    j.at("nonce").get_to(block.nonce);
    j.at("previous_hash").get_to(block.previous_hash);
}


class Blockchain {
    using Chain = std::vector<Block>;
    using Nodes = std::set<std::string>;
    using Transactions = std::vector<Transaction>;

    Transactions unconfirmed_transactions;
    Chain chain;
    Nodes nodes;
    std::string my_node;
    bool log_mode = false;
public:
    explicit Blockchain(std::string node) : my_node(std::move(node)) {
        chain.push_back(new_block("1"));
    }

    Block new_block(std::string previous_hash) {
        Block block = {
                chain.size() + 1,
                std::time(nullptr),
                std::move(unconfirmed_transactions),
                0,
                std::move(previous_hash)
        };

        return block;
    }

    void register_node(const std::string &node) {

        if (nodes.insert(node).second) {
            log("register node:\n" + json(node).dump() + '\n');

            json request{
                    {"nodes", std::vector{my_node}}
            };
            Uri parsed = Uri::Parse(node);
            httplib::Client cli{parsed.Host, stoi(parsed.Port)};
            cli.Post("/nodes/register", request.dump(), "text/plane");
        }
    }

    Block mine() {

        log("start mining");
        std::string previous_hash = Blockchain::hash(last_block());
        Block block = new_block(previous_hash);
        size_t chain_size = chain.size();
        while (!hash_is_valid(hash(block))) {
            if(chain_size != chain.size()) {
                block.previous_hash = Blockchain::hash(last_block());
            }
            block.nonce++;
        }

        chain.push_back(block);

        log("finish mining");
        log("block mined:\n" + json(block).dump(4) + '\n');;

        for (const auto &node: nodes) {

            Uri parsed = Uri::Parse(node);
            httplib::Client cli(parsed.Host, stoi(parsed.Port));
            cli.Get("/nodes/resolve");
        }

        return block;

    }

    static bool hash_is_valid(const std::string &hash) {
        return hash.substr(0, 4) == "0000";
    }

    static bool is_valid(Chain &chain) {

        for (size_t i = 1; i < chain.size(); i++) {
            Block &last_block = chain[i - 1];
            Block &block = chain[i];

            if (block.previous_hash != hash(last_block) ||
                !hash_is_valid(hash(block))) {
                return false;
            }
        }

        return true;

    }


    bool resolve_conflicts() {
        log("resolving conflict");

        size_t max_len = 0;
        Chain max_chain;
        std::string max_node;
        for (const auto &node: nodes) {

            Uri parsed = Uri::Parse(node);
            httplib::Client cli(parsed.Host, stoi(parsed.Port));

            auto res = cli.Get("/chain");
            if (res && res->status == 200) {
                json response = json::parse(res->body);

                size_t len = response["length"];
                Chain other_chain = response["chain"];
                if (len > max_len && is_valid(other_chain)) {
                    max_len = len;
                    max_chain = std::move(other_chain);
                    max_node = node;
                }
            }
        }
        if (max_len > chain.size() || !is_valid(chain)) {
            chain = std::move(max_chain);
            for (const auto &node: nodes) {

                Uri parsed = Uri::Parse(node);
                httplib::Client cli(parsed.Host, stoi(parsed.Port));
                cli.Get("/nodes/resolve");
            }
            log("chain was replaced from " + max_node +'\n');
            return true;
        }
        log("chain is authoritative\n");
        return false;
    }


    static std::string hash(const Block &block) {
        json json_block = block;
        return picosha2::hash256_hex_string(json_block.dump());
    }

    void new_transaction(Transaction transaction) {
        log("new transaction:\n" + json(transaction).dump() + '\n');

        unconfirmed_transactions.push_back(std::move(transaction));
//
//        for(auto& node : nodes) {
//            Uri parsed = Uri::Parse(node);
//            httplib::Client cli(parsed.Host, stoi(parsed.Port)); // TODO check stoi
//            cli.Post("/transactions/new", request.dump(), "text/plane");
//        }
    }


    Block &last_block() {
        return chain.back();
    }

    const Chain &get_chain() { return chain; }

    const Nodes &get_nodes() { return nodes; }

    const Transactions &get_transactions() { return unconfirmed_transactions; }


    void log_on() { log_mode = true; }

    void log_off() { log_mode = false; }

private:

    static void log(const std::string &input) {
        std::cout << input << std::endl;
    }
};


#endif //MYCOIN_BLOCKCHAIN_H
