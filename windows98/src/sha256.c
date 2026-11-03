/* SPDX-License-Identifier: GPL-3.0-or-later
 * Portable SHA-256. No CryptoAPI, 64-bit integer helpers or CRT imports.
 */
#include "d3core.h"
static const D3U32 K[64]={
0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
static D3U32 rr(D3U32 x,int n){return (x>>n)|(x<<(32-n));}
static D3U32 be(const D3U8 *p){return ((D3U32)p[0]<<24)|((D3U32)p[1]<<16)|((D3U32)p[2]<<8)|p[3];}
static void block(D3U32 h[8],const D3U8 *p){
    D3U32 w[64],a,b,c,d,e,f,g,t,x,y;int i;
    for(i=0;i<16;i++)w[i]=be(p+4*i);
    for(i=16;i<64;i++){x=rr(w[i-15],7)^rr(w[i-15],18)^(w[i-15]>>3);y=rr(w[i-2],17)^rr(w[i-2],19)^(w[i-2]>>10);w[i]=w[i-16]+x+w[i-7]+y;}
    a=h[0];b=h[1];c=h[2];d=h[3];e=h[4];f=h[5];g=h[6];t=h[7];
    for(i=0;i<64;i++){x=t+(rr(e,6)^rr(e,11)^rr(e,25))+((e&f)^((~e)&g))+K[i]+w[i];y=(rr(a,2)^rr(a,13)^rr(a,22))+((a&b)^(a&c)^(b&c));t=g;g=f;f=e;e=d+x;d=c;c=b;b=a;a=x+y;}
    h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=t;
}
void d3_sha256(const D3U8 *data,D3U32 size,D3U8 digest[32]){
    D3U32 h[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    D3U8 tail[128];D3U32 n=size,pos=0,remaining,padded,hi=size>>29,lo=size<<3;int i;
    while(n>=64){block(h,data+pos);pos+=64;n-=64;}
    d3_zero(tail,128);remaining=n;d3_copy(tail,data+pos,remaining);tail[remaining]=0x80;padded=remaining<56?64:128;
    for(i=0;i<4;i++){tail[padded-8+i]=(D3U8)(hi>>(24-8*i));tail[padded-4+i]=(D3U8)(lo>>(24-8*i));}
    block(h,tail);if(padded==128)block(h,tail+64);
    for(i=0;i<32;i++)digest[i]=(D3U8)(h[i/4]>>(24-8*(i%4)));
}
