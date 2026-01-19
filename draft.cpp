#include "kawa/core/core.h"
#include "kawa/core/experimental.h"

#include "kawa/core/multi_broadcaster.h"

using namespace kawa;

struct event { i32 value; };

struct sub
{
    sub(multi_broadcaster& mb)
        : listner{this, mb.get<event>()}
    {

    }

    broadcaster<event>::listner listner;
    void recieve(const event& e)
    {
        kw_info("got event {}", e.value);
    }
       

};

int main()
{
    multi_broadcaster bc;
    sub s(bc);

    bc.emit<event>({42});
}