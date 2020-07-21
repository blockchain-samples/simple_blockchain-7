#include <iostream>
#include <vector>
#include <set>
#include <ctime>



#include "blockchain.h"

using nlohmann::json;

bool always_mine = false;

int main() {

    httplib::Server svr;
    size_t port;
    std::cout << "Enter port" << std::endl;
    std::cin >> port;


    Blockchain blockchain("localhost:" + std::to_string(port));
    blockchain.log_on();
    std::cout << "blockchain started" << std::endl;


    svr.Get("/mine", [&blockchain](const httplib::Request &req, httplib::Response &res) {
        Block block = blockchain.mine();
        res.set_content(json(block).dump(4), "text/plain");
    });

    svr.Get("/mine/always", [&blockchain](const httplib::Request &req, httplib::Response &res) {
        always_mine = true;
        size_t block_count = 0;
        while (always_mine) {
            Block block = blockchain.mine();
            block_count++;
        }
        res.set_content("mined " + std::to_string(block_count) + " blocks", "text/plain");
    });

    svr.Get("/mine/stop", [&blockchain](const httplib::Request &req, httplib::Response &res) {
        always_mine = false;
    });





    svr.Get("/transactions", [&blockchain](const httplib::Request &req, httplib::Response &res) {
        res.set_content(json(blockchain.get_transactions()).dump(4), "text/plane");
    });

    svr.Post("/transactions/new", [&blockchain](const httplib::Request &req, httplib::Response &res) {
        json request = json::parse(req.body);

        blockchain.new_transaction(request.get<Transaction>());

        json response;
        response["message"] = "Transaction will be in block " + std::to_string(blockchain.get_chain().size() + 1);
        res.set_content(response.dump(4), "text/plane");
    });





    svr.Get("/chain", [&blockchain](const httplib::Request &req, httplib::Response &res) {
        json response;
        response["chain"] = blockchain.get_chain();
        response["length"] = blockchain.get_chain().size();
        res.set_content(response.dump(4), "text/plane");
    });


    svr.Post("/nodes/register", [&blockchain](const httplib::Request &req, httplib::Response &res) {
        json request = json::parse(req.body);

        std::vector<std::string> nodes = request["nodes"];


        for (auto &node : nodes) {
            blockchain.register_node(node);
        }

        json response;
        response["message"] = "New nodes have been added";
        response["total_nodes"] = blockchain.get_nodes();
        res.set_content(response.dump(), "text/plane");
    });

    svr.Get("/nodes/resolve", [&blockchain](const httplib::Request &req, httplib::Response &res) {
        bool replaced = blockchain.resolve_conflicts();

        json response;
        response["message"] = replaced ? "Our chain was replaced" : "Our chain is authoritative";
        response["chain"] = blockchain.get_chain();


        res.set_content(response.dump(4), "text/plane");
    });

    svr.Get("/nodes", [&blockchain](const httplib::Request &req, httplib::Response &res) {
        json response = {
                {"nodes", blockchain.get_nodes()}
        };

        res.set_content(response.dump(), "text/plane");
    });


    svr.listen("localhost", port);
    return 0;
}
