#include "RoundedMask.hpp"
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

struct Pixel { std::uint8_t alpha, red, green, blue; };
constexpr int W=960, H=540;
Pixel source(int x,int y) {
    if(y<108 || y>=459) return {255,0,0,0};
    const double t=double(y-108)/351;
    Pixel p{255,static_cast<std::uint8_t>(79+123*t),static_cast<std::uint8_t>(130-27*t),
                static_cast<std::uint8_t>(179-46*t)};
    if((x-680)*(x-680)+(y-193)*(y-193)<66*66) p={255,255,217,157};
    if(y>345-67*std::sin(x/164.0)) p={255,75,87,125};
    if(y>395-47*std::sin(x/122.0+1.4)) p={255,39,68,87};
    if(y>460-40*std::sin(x/192.0+2.8)) p={255,21,47,61};
    // Fixed, evenly spaced markers make any accidental content stretch visible.
    for(int row=0;row<2;++row) for(int col=0;col<3;++col) {
        int cx=154+col*48,cy=224+row*48;
        if((x-cx)*(x-cx)+(y-cy)*(y-cy)<8*8) p={255,217,239,229};
    }
    return p;
}
void save(const std::string& path,const rounded::Settings& s,bool original=false) {
    std::ofstream out(path,std::ios::binary);
    if(!out) throw std::runtime_error("Cannot write " + path);
    out<<"P6\n"<<W<<' '<<H<<"\n255\n";
    rounded::Mask mask(W,H,s);
    for(int y=0;y<H;y++) for(int x=0;x<W;x++) {
        auto p=source(x,y);
        double a=original ? 1 : mask.coverage(x,y);
        auto cut=rounded::apply(p,a);
        int bg=((x/18+y/18)%2)?39:32;
        const char bytes[]={static_cast<char>(cut.red+bg*(1-a)),
                            static_cast<char>(cut.green+(bg+4)*(1-a)),
                            static_cast<char>(cut.blue+(bg+8)*(1-a))};
        out.write(bytes,3);
    }
}
int main(int argc,char**argv) {
    if(argc!=2)return 2;
    std::string dir=argv[1];
    rounded::Settings s;
    save(dir+"/source.ppm",s,true);
    s.radius=15;s.top=20;s.bottom=15;
    save(dir+"/trimmed.ppm",s);
    s.left=35;s.right=5;
    save(dir+"/portrait.ppm",s);
    s.left=5;s.right=5;s.top=30;s.bottom=25;
    save(dir+"/wide.ppm",s);
    s={};s.top=20;s.bottom=15;
    for(int r:{0,10,25,50}) {s.radius=r;save(dir+"/radius-"+std::to_string(r)+".ppm",s);}
    std::cout<<"Rendered documentation fixtures with RoundedMask.hpp\n";
}
