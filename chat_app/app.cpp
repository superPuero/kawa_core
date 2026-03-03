//#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "../httplib.h"
#include "../kawa/core/core.h"
#include "../json.h"

#include <string>
#include <unordered_map>
#include <random>

using namespace kawa;
using namespace std;

struct message
{
    entity_id from;
    string content;
};

//template<>
//struct registry::serialize_trait<user>
//{
//    static void write(registry& reg, entity_id id, std::ofstream& out)
//    {
//        auto& value = reg.get<user>(id);
//        out << value.name << '\n';
//        out << value.passhash << '\n';
//    }
//};
//
//template<>
//struct registry::deserialize_trait<user>
//{
//    static void read(registry& reg, entity_id id, std::ifstream& in)
//    {
//        user value;
//        in >> value.name;
//        in >> value.passhash;
//        reg.add(id, std::move(value));
//    }
//};

struct chat
{
    string name;
    dyn_array<message> messages;
};
    
struct user
{
    string name;
    u64 passhash;
};

template<>
struct registry::serialize_trait<user>
{
    static void write(registry& reg, entity_id id, std::ofstream& out)
    {
        auto& value = reg.get<user>(id);
        out << value.name << '\n';
        out << value.passhash << '\n';
    }
};

template<>
struct registry::deserialize_trait<user>
{
    static void read(registry& reg, entity_id id, std::ifstream& in)
    {
        user value;
        in >> value.name;
        in >> value.passhash;
        reg.add(id, std::move(value));
    }
};


std::mutex session_map_mutex;

umap<string, entity_id> session_map;

string get_session_id(const string& cookie)
{
    constexpr string_view key = "session_id=";
    thread_local string out;
    out.clear();

    auto pos = cookie.find(key);

    if (pos != string::npos)
    {
        out = cookie.substr(pos + key.length());

        auto end_pos = out.find(';');

        if (end_pos != string::npos)
        {
            out = out.substr(0, end_pos);
        }
    }

    return out;
}

void reset_cookie(httplib::Response& res)
{
    res.headers.erase("Set-Cookie");
    res.headers.emplace("Set-Cookie", "session_id=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT");
}

bool verify_cookie(const httplib::Request& req)
{
    if (!req.has_header("Cookie")) { return false; }

    thread_local string cookie;
    thread_local string session_id;
    cookie.clear();
    session_id.clear();

    cookie = req.get_header_value("Cookie");
    if (cookie.empty()) { return false; }

    session_id = get_session_id(cookie);

    if (session_id.empty()) { return false; }
    if (!session_map.contains(session_id)) { return false; }
}

void make_cookie(httplib::Response& res, entity_id user_id)
{
    kw_assert(user_id != nullent);

    string session_id = std::to_string(rng::randu64());

    {
        std::lock_guard g(session_map_mutex);
        session_map[session_id] = user_id;
    }

    res.headers.emplace("Set-Cookie", std::format("session_id={}; Path=/; HttpOnly", session_id));
}

int main() 
{   
    registry user_db({ .name = "users", .max_entity_count = 10'000, .max_component_count = 16 });

    user_db.ensure<user>();
    std::ifstream in("users.kwdb");
    user_db.deserialize(in);

    registry chats_db({ .name = "chats", .max_entity_count = 1'000'000, .max_component_count = 2 });

    //chats_db.ensure<message>();
    //std::ifstream in("chats.kwdb");
    //chats_db.deserialize(in);

    httplib::Server svr;

    /*httplib::SSLServer svr(
        "/etc/letsencrypt/live/kaw4.com/fullchain.pem",
        "/etc/letsencrypt/live/kaw4.com/privkey.pem"
    );*/

    if (!svr.is_valid())
    {
        kw_info("Error: Server certificate/key is invalid.");
        return 1;
    }

    svr.set_mount_point("/", "./chat_app");

    svr.Get("/", [](const httplib::Request& rq, httplib::Response& res)
        {
            res.set_redirect("/chat");
        }
    );

    svr.Get("/chat", [](const httplib::Request& rq, httplib::Response& res)
        {            
            res.set_file_content("./chat_app/chat.html", "text/html");
        }
    );

    svr.Get("/try_login",
        [&](const httplib::Request& req, httplib::Response& res)
        {
            if (!verify_cookie(req)) { res.status = httplib::Unauthorized_401; return; }
            res.status = httplib::OK_200;
        }
    );

    svr.Post("/logout",
        [&](const httplib::Request& req, httplib::Response& res)
        {
            reset_cookie(res);
        }
    );

    svr.Post("/login",
        [&](const httplib::Request& req, httplib::Response& res)
        {
            nlohmann::json body = nlohmann::json::parse(req.body, nullptr, false);
            if (body.empty() || !body.is_object() || !body.contains("username") || !body.contains("password"))
            {
                res.status = httplib::BadRequest_400;
                res.set_content(R"({"error": "invalid request body"})", "application/json");
                return;
            }

            auto username = body["username"].get<string_view >();
            auto passhash = fnv1a_hash(body["password"].get<string_view>());

            enum {
                success,
                user_not_found,
                incorrect_password
            } result = user_not_found;

            entity_id user_id = nullent;

            { // TRANSACTION: user
                auto tr = user_db.lock_guard<user>();


                user_db.query_until(
                    [&](entity_id id, user& u)
                    {
                        if (u.name == username)
                        {
                            if (u.passhash == passhash)
                            {
                                user_id = id;
                                result = success;
                            }
                            else
                            {
                                result = incorrect_password;
                            }

                            return true;
                        }
                    
                        result = user_not_found;
                        return false;
                    }
                );
            }
            
            switch (result)
            {
            case success:
            {
                make_cookie(res, user_id);
                res.set_content(R"({"status": "success"})", "application/json");
            }
            break;
            case user_not_found:
                res.status = httplib::NotFound_404; 
                res.set_content(R"({"error": "user not found"})", "application/json");
                break;
            case incorrect_password:
                res.status = httplib::Unauthorized_401;
                res.set_content(R"({"error": "incorrect password"})", "application/json");
                break;
            default:
                res.status = httplib::InternalServerError_500;
                res.set_content(R"({"error": "internal server error"})", "application/json");
                break;
            }
        }
    );

    svr.Post("/register",
        [&](const httplib::Request& req, httplib::Response& res)
        {
            nlohmann::json body = nlohmann::json::parse(req.body);

            auto username = body["username"].get<string_view>();

            { // TRANSACTION: user
                auto tr = user_db.lock_guard<user>();

                bool already_exists = false;            

                user_db.query_until(
                    [&](entity_id id, user& u)
                    {
                        if (u.name == username)
                        {
                            already_exists = true;
                            return true;
                        }

                        return false;
                    }
                );

                if (already_exists)
                {
                    res.status = httplib::Conflict_409; 
                    res.set_content(R"({"error": "user already exists"})", "application/json");
                }
                else
                {
                    auto id = user_db.entity(
                        user{ .name{body["username"].get<string_view>()}, .passhash = fnv1a_hash(body["password"].get<string_view>()) }
                    );
                    res.status = httplib::Created_201; 
                    make_cookie(res, id);
                    res.set_content(R"({"status": "success"})", "application/json");
                }
            }
        }
    );


    svr.Get("/stop", 
        [&](const httplib::Request& req, httplib::Response& res)
        {
            svr.stop();
        }
    );   

    svr.Post("/send_message", 
        [&](const httplib::Request& req, httplib::Response& res)
        {
            nlohmann::json body = nlohmann::json::parse(req.body, nullptr, false);
            if (body.empty() || !body.is_object() || !body.contains("input"))
            {
                res.status = httplib::BadRequest_400;
                res.set_content(R"({"error": "invalid request body"})", "application/json");
                return;
            }

            if (!verify_cookie(req))
            {
                res.status = httplib::Unauthorized_401;
                reset_cookie(res);
                res.set_redirect("/chat");
                return;
            }

            string cookie = req.get_header_value("Cookie");
            string session_id = get_session_id(cookie);

            if (!session_map.contains(session_id))
            {
                reset_cookie(res);
                return;
            }
            else
            {
                auto id = session_map[session_id];
                auto tr = chats_db.lock_guard<message>();

                chats_db.entity(
                    message{ .from = id, .content{body["input"].get<string_view>()} }
                );
            }
        }
    );

    svr.Get("/get_messages",
        [&](const httplib::Request& req, httplib::Response& res)
        {
            thread_local string out;
            out.clear();

            if (!verify_cookie(req)) { reset_cookie(res); res.status = httplib::BadRequest_400; return; }

            string cookie = req.get_header_value("Cookie");
            string session_id = get_session_id(cookie);

            entity_id user_id = nullent;

            {
                std::lock_guard g(session_map_mutex); 
                if (session_map.contains(session_id)) 
                {
                    user_id = session_map[session_id];
                }
                else
                {
                    res.status = httplib::Unauthorized_401;
                    reset_cookie(res);
                    return;
                }
            }

            if (user_id == nullent)
            {
                res.status = httplib::Unauthorized_401;
                reset_cookie(res);
                return;
            }

            auto tr1 = chats_db.lock_guard<message>();
            auto tr2 = user_db.lock_guard<user>();

            chats_db.query(
                [&](message& msg)
                {
                    if (msg.from == user_id)
                    {
                        out += std::format("<li class=\"message mine\">{}</li>", msg.content);
                    }
                    else
                    {
                        out += std::format("<li class=\"message theirs\"><span class=\"sender-name\">{}</span>: {}</li>",
                            user_db.get<user>(msg.from).name,
                            msg.content);
                    }
                }
            );

            res.set_content(out, "text/html");
        }
    );

    //svr.listen("0.0.0.0", 443);

    svr.listen("0.0.0.0", 8080);

    std::ofstream out("users.kwdb");
    user_db.serialize(out);

}