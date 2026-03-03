#include "kawa/core/core.h"
#include "httplib.h"

using namespace kawa;

struct client
{
    u64 value;
};



int main()
{
    registry db({ .name = "kawa_db", .max_entity_count = 100, .max_component_count = 32 });
    atomic<u32> transaction_count = 0;

    for (usize i = 0; i < 100; i++)
    {
        db.entity(client{ 100'000 });
    }

    httplib::Server svr;
    
    svr.Get("/test", [&](const httplib::Request& rq, httplib::Response& res)
        {
            db.transaction<client>(
                [&](registry& trdb)
                {
                    trdb.query_with((entity_id)(rng::randu64() % 100),
                        [&](client& client_from) 
                        {
                            u64 value = rng::randu64() % 5000;

                            trdb.query_with((entity_id)(rng::randu64() % 100),
                                [&](client& client_to)
                                {
                                    if (client_from.value >= value)
                                    {
                                        client_from.value -= value;
                                        client_to.value += value;
                                    }
                                }
                            );
                        }
                    );

                    transaction_count++;
                }
            );
        }
    );

    svr.Get("/sum",
        [&](const httplib::Request& rq, httplib::Response& res)
        {
            u64 sum = 0;
            db.transaction<client>(
                [&](registry& trdb)
                {
                    trdb.query([&](client& c) { sum += c.value; });
                }
            );
            res.set_content(std::to_string(sum) + "\n" + std::to_string(transaction_count.load()), "text/plain");
        }
    );


    svr.listen("0.0.0.0", 8080);
    
}