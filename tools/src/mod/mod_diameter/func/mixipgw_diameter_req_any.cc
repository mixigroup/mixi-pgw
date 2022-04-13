//
// Created by mixi on 2017/10/18.
//

#include "../mixipgw_mod_diameter.hpp"
#include "../mixipgw_mod_diameter.inl"

using namespace MIXIPGW_TOOLS;

namespace COUNTING{
  static const unsigned TRANSFER_RING_BURST = 128;/*!< burst size to transfer */
  // counting : 1  item 
  typedef struct counting_item {
      uint32_t    key:32;
      uint32_t    used:32;
  } __attribute__ ((packed)) counting_item_t,*counting_item_ptr;
  static_assert(sizeof(struct counting_item)==8, "must 8 octet");
  // counting : 1 packet 
  typedef struct counting{
      uint32_t    type:8;
      uint32_t    len:24;
      uint32_t    count:32;
      uint32_t    reserved:32;
      counting_item_t   items[TRANSFER_RING_BURST];
  } __attribute__ ((packed)) counting_t,*counting_ptr;
  static_assert(sizeof(struct counting)==1024+12, "must 1024+12 octet");
};

using namespace COUNTING;

int Diameter::ReqAny(void* dh, evutil_socket_t sock, void* p){
    auto cli = (DiameterClient*)p;
    int ret = RETERR;
    auto c = (counting_ptr)dh;
    //
    std::string sql = "INSERT INTO log_diameter_gy(imsi,used_s5_bytes,used_sgi_bytes,reporter)VALUES";

    // prepare bulk insert SQL.
    for(auto n = 0;n < c->count;n++){
        char bf[128] = {0};
        uint32_t used_s5 = 0;
        uint32_t used_sgi= 0;
        if (c->type == DIAMETER_VERSION_COUNTER_INGRESS){
            used_sgi = c->items[n].used;
        }else{
            used_s5 = c->items[n].used;
        }
        snprintf(bf, sizeof(bf) - 1,"%s(%u,%u,%u,'c')",
                 n==0?"":",",
                 (unsigned)c->items[n].key,
                 (unsigned)used_s5,
                 (unsigned)used_sgi);
        sql += bf;
    }
    if (cli->mysql_->Query(sql.c_str()) != RETOK){
        Logger::LOGERR("failed. query(%s)",sql.c_str());
        return(RETERR);
    }
    return(RETOK);
}

