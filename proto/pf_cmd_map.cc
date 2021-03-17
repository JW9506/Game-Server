#include "pf_cmd_map.h"
#include "proto_man.h"

char* pf_cmd_map[] = {
    "LoginReq",
    "LoginRes",
};

void init_pf_cmd_map() {
    proto_man::register_pf_cmd_map(pf_cmd_map,
                                   sizeof(pf_cmd_map) / sizeof(char*));
}
