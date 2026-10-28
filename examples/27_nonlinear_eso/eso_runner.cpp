#include "adrc/nonlinear_eso3.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <string>
int main(int argc,char** argv)
{
    if(argc!=4) return 1;
    std::ifstream config(argv[1]),input(argv[2]); std::ofstream output(argv[3]);
    adrc::NonlinearESO3Config c={}; float z1=0,z2=0,z3=0;
    if(!(config>>c.b0>>c.beta1>>c.beta2>>c.beta3>>c.alpha1>>c.alpha2>>c.delta>>z1>>z2>>z3)) return 1;
    config>>std::ws; if(!config.eof()) return 1;
    adrc::NonlinearESO3 observer;
    if(!observer.Init(c,z1,z2,z3) || !input || !output) return 1;
    std::string line; if(!std::getline(input,line)) return 1;
    if(!line.empty() && line.back()=='\r') line.pop_back();
    if(line!="time,input,measurement,dt") return 1;
    output<<std::setprecision(17)<<"time,input,measurement,dt,z1,z2,z3,next_z1,next_z2,next_z3\n";
    unsigned count=0;
    while(std::getline(input,line)) {
        std::istringstream row(line); double t=0; float u=0,y=0,h=0; char a=0,b=0,d=0;
        if(!(row>>t>>a>>u>>b>>y>>d>>h) || a!=',' || b!=',' || d!=',' || !std::isfinite(t)) return 1;
        row>>std::ws; if(!row.eof()) return 1;
        z1=observer.GetOutput(); z2=observer.GetRate(); z3=observer.GetDisturbance();
        if(!observer.Update(u,y,h)) return 1;
        output<<t<<','<<u<<','<<y<<','<<h<<','<<z1<<','<<z2<<','<<z3<<','
            <<observer.GetOutput()<<','<<observer.GetRate()<<','<<observer.GetDisturbance()<<'\n';
        ++count;
    }
    output.flush(); if(input.bad() || !output || count==0) return 1;
    std::cout<<"PASS: nonlinear ESO rows="<<count<<'\n';
}
