#include "compress.h"
#define HASH_SIZE 4096
#define MIN_MATCH 4
#define MAX_MATCH 18
#define MAX_DIST  4095
static uint32_t h4(const uint8_t* p) {
    uint32_t v = p[0]|(p[1]<<8)|(p[2]<<16)|(p[3]<<24);
    return (v*2654435761U)>>20;
}
int compress_page(const uint8_t* src, size_t slen, uint8_t* dst, size_t dlen) {
    uint16_t ht[HASH_SIZE];
    for(int i=0;i<HASH_SIZE;i++) ht[i]=0;
    const uint8_t* se=src+slen, *de=dst+dlen;
    const uint8_t* ip=src, *ls=src;
    uint8_t* op=dst;
#define FLUSH() do{ \
    size_t ll=ip-ls; \
    while(ll>0){ size_t r=ll>128?128:ll; if(op+1+r>de) return COMPRESS_FAIL; \
        *op++=(uint8_t)(r-1); for(size_t _i=0;_i<r;_i++) *op++=ls[_i]; ls+=r; ll-=r; } \
}while(0)
    while(ip+MIN_MATCH<=se){
        uint32_t hv=h4(ip)&(HASH_SIZE-1);
        uint16_t ref=ht[hv]; ht[hv]=(uint16_t)(ip-src);
        const uint8_t* m=src+ref;
        int32_t d=(int32_t)(ip-m);
        if(d>0&&d<=MAX_DIST&&ip[0]==m[0]&&ip[1]==m[1]&&ip[2]==m[2]&&ip[3]==m[3]){
            int len=MIN_MATCH;
            while(len<MAX_MATCH&&ip+len<se&&ip[len]==m[len]) len++;
            FLUSH();
            if(op+2>de) return COMPRESS_FAIL;
            int el=len-MIN_MATCH, off=d-1;
            *op++=(uint8_t)(0x80|(el<<3)|(off>>8));
            *op++=(uint8_t)(off&0xFF);
            ip+=len; ls=ip;
        } else ip++;
    }
    ip=se; FLUSH();
#undef FLUSH
    int cs=(int)(op-dst);
    return cs>=(int)slen ? COMPRESS_FAIL : cs;
}
int decompress_page(const uint8_t* src, size_t slen, uint8_t* dst, size_t dlen) {
    const uint8_t* ip=src, *se=src+slen;
    uint8_t* op=dst, *de=dst+dlen;
    while(ip<se){
        uint8_t t=*ip++;
        if(!(t&0x80)){
            int n=(t&0x7F)+1;
            if(ip+n>se||op+n>de) return -1;
            for(int i=0;i<n;i++) *op++=*ip++;
        } else {
            if(ip>=se) return -1;
            uint8_t b2=*ip++;
            int len=((t>>3)&0xF)+MIN_MATCH;
            int off=(((t&7)<<8)|b2)+1;
            uint8_t* mp=op-off;
            if(mp<dst||op+len>de) return -1;
            for(int i=0;i<len;i++) *op++=mp[i];
        }
    }
    return (int)(op-dst);
}
