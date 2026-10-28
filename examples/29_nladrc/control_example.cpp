// 主机完整组合：非线性TD + 三阶NLESO + 两项fal反馈。生产核心无对象/日志依赖。
#include "adrc/adrc.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
int main(int argc,char** argv)
{
    if(argc!=4) return 1;
    std::ifstream config(argv[1]),input(argv[2]); std::ofstream output(argv[3]);
    adrc::NLESO3Config c={}; float b1=0,a1=0,b2=0,a2=0,delta=0,r=0,h0=0,limit=0;
    if(!(config>>c.b0>>c.beta1>>c.beta2>>c.beta3>>c.alpha1>>c.alpha2>>c.delta
        >>b1>>a1>>b2>>a2>>delta>>r>>h0>>limit)) return 1;
    config>>std::ws;
    if(!config.eof() || !std::isfinite(limit) || limit<0) return 1;
    adrc::NLTD2 td; adrc::NLESO3 observer; adrc::NLSEF2 feedback;
    if(!td.Init(r,h0) || !observer.Init(c) || !feedback.Init(c.b0,b1,a1,b2,a2,delta) || !input || !output) return 1;
    std::string line; if(!std::getline(input,line)) return 1;
    if(!line.empty() && line.back()=='\r') line.pop_back();
    if(line!="time,target,force,noise,dt") return 1;
    output<<std::setprecision(17)<<"time,target,force,noise,dt,measurement,true_y,true_rate,td1,td2,z1,z2,z3,requested,applied,next_td1,next_td2,next_z1,next_z2,next_z3,next_y,next_rate\n";
    double y=0,v=0; unsigned count=0;
    while(std::getline(input,line)) {
        std::istringstream row(line); double t=0,noise=0; float target=0,force=0,h=0; char a=0,b=0,d=0,e=0;
        if(!(row>>t>>a>>target>>b>>force>>d>>noise>>e>>h) || a!=',' || b!=',' || d!=',' || e!=',' ||
            !std::isfinite(t) || !std::isfinite(force) || !std::isfinite(noise)) return 1;
        row>>std::ws; if(!row.eof()) return 1;
        // 1. 当前输出测量；不向反馈或ESO传入真实扰动和真实速度。
        const float measurement=static_cast<float>(y+noise);
        // 2. 读取TD/ESO当前状态；TD变化率作为目标速度，而不是原始阶跃的导数。
        const float td1=td.GetOutput(),td2=td.GetRate();
        const float z1=observer.GetOutput(),z2=observer.GetRate(),z3=observer.GetDisturbance();
        float requested=0;
        if(!feedback.Calc(td1,td2,z1,z2,z3,requested)) return 1;
        // 3. 调用方最终限幅。硬件工程在此提交最终实际作用量。
        const float applied=limit>0 ? std::max(-limit,std::min(limit,requested)) : requested;
        // 4. 每区间更新ESO与TD各一次，右侧均为本拍量，更新值供下一拍使用。
        if(!observer.Update(applied,measurement,h) || !td.Update(target,h)) return 1;
        // 5. 仅主机：精确ZOH二阶教学对象。实际输入与观测器所用输入完全相同。
        const double acceleration=static_cast<double>(c.b0)*applied+force,dt=h;
        const double ny=y+dt*v+0.5*dt*dt*acceleration,nv=v+dt*acceleration;
        if(!std::isfinite(ny) || !std::isfinite(nv)) return 1;
        output<<t<<','<<target<<','<<force<<','<<noise<<','<<h<<','<<measurement<<','<<y<<','<<v<<','
            <<td1<<','<<td2<<','<<z1<<','<<z2<<','<<z3<<','<<requested<<','<<applied<<','
            <<td.GetOutput()<<','<<td.GetRate()<<','<<observer.GetOutput()<<','<<observer.GetRate()<<','
            <<observer.GetDisturbance()<<','<<ny<<','<<nv<<'\n';
        y=ny;v=nv;++count;
    }
    output.flush(); if(input.bad() || !output || count==0) return 1;
    std::cout<<"PASS: full NLADRC rows="<<count<<'\n';
}
