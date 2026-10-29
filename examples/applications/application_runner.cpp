// 四类应用的自主C++仿真。仅此例程持有对象模型、PID、角度处理、延迟和功率限制。
// 这些参数都是可复现实验的假设，不是任何实车的推荐参数。
#include "ALG_ADRC.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
const double pi=3.14159265358979323846;
struct Parameters {
    float b0,wc,wo,limit;
    double damping,tau,initial;
    int wheels,variants;
};
Parameters parameters(int id) {
    if(id==1) return {12,12,45,2,3,0.01,0,1,3};
    if(id==2) return {30,8,30,1.5f,0.8,0.008,179*pi/180,1,3};
    if(id==3) return {30,8,30,1.5f,0.8,0.008,0,1,4};
    return {10,12,40,3,1.6,0.01,0,4,3};
}
void require(bool ok) {if(!ok) throw std::runtime_error("algorithm or fixture validation failed");}
float clip(float x,float limit) {return std::max(-limit,std::min(limit,x));}
double wrap(double x) {return x-2*pi*std::floor((x+pi)/(2*pi));}
struct AngleStream {
    float last=0,lifted=0;
    void init(double x) {last=static_cast<float>(wrap(x));lifted=static_cast<float>(x);}
    float update(float raw) {
        float delta=raw-last;
        while(delta>static_cast<float>(pi)) delta-=static_cast<float>(2*pi);
        while(delta<-static_cast<float>(pi)) delta+=static_cast<float>(2*pi);
        lifted+=delta;last=raw;return lifted;
    }
};
struct Node {
    ADRC::LADRC1 speed;
    ADRC::LADRC2 angle;
    ADRC::ESO2 eso2;
    ADRC::NLESF1 nlesf1;
    ADRC::ESO3 eso3;
    ADRC::NLESF2 nlesf2;
    ADRC::LTD2 td;
    AngleStream target_stream,measurement_stream;
    double y=0,v=0,actuator=0;
    std::array<double,2> delayed{{0,0}};
    float integral=0,last_applied=0;
    void init(const Parameters& p) {
        y=p.initial; target_stream.init(y);measurement_stream.init(y);
        const float initial=static_cast<float>(y),delta=0.02f;
        require(speed.Init(p.b0,p.wc,p.wo,initial));
        require(angle.Init(p.b0,p.wc,p.wo,initial));
        require(eso2.Init({p.b0,2*p.wo,p.wo*p.wo*std::pow(delta,0.5f),0.5f,delta},initial));
        require(nlesf1.Init(p.b0,p.wc*std::pow(0.1f,0.25f),0.75f,0.1f));
        require(eso3.Init({p.b0,3*p.wo,3*p.wo*p.wo*std::pow(delta,0.5f),
                p.wo*p.wo*p.wo*std::pow(delta,0.75f),0.5f,0.25f,delta},initial));
        require(nlesf2.Init(p.b0,p.wc*p.wc*std::pow(0.1f,0.25f),0.75f,
                2*p.wc*std::pow(0.1f,-0.25f),1.25f,0.1f));
        require(td.Init(12,initial));
    }
};
std::array<float,3> state(const Node& n,int id,int variant) {
    if(variant==0) return {{0,0,0}};
    if(id==1 && variant==2) return {{n.eso2.GetOutput(),0,n.eso2.GetDisturbance()}};
    if(id==1 || id==4) return {{n.speed.GetEstimatedOutput(),0,n.speed.GetEstimatedDisturbance()}};
    if(id==3 && variant==3) return {{n.eso3.GetOutput(),n.eso3.GetRate(),n.eso3.GetDisturbance()}};
    return {{n.angle.GetEstimatedOutput(),n.angle.GetEstimatedRate(),n.angle.GetEstimatedDisturbance()}};
}
// 教学功率模型：P(lambda)=copper*lambda^2+mechanical*lambda；不是裁判系统规则或实车功率模型。
float power_scale(const std::array<float,4>& command,const std::array<Node,4>& nodes) {
    double copper=0,mechanical=0;
    for(int j=0;j<4;++j) {copper+=2.0*command[j]*command[j];mechanical+=0.4*std::fabs(command[j]*nodes[j].y);}
    if(copper+mechanical<=12.0) return 1;
    const double scale=24.0/(mechanical+std::sqrt(mechanical*mechanical+48.0*copper));
    return static_cast<float>(std::min(1.0,scale));
}
}

int main(int argc,char** argv) {
    try {
        if(argc!=4) {std::cerr<<"usage: application_runner case_id fixture.csv output.csv\n";return 2;}
        const int id=std::atoi(argv[1]);require(id>=1 && id<=4);
        const Parameters p=parameters(id);
        std::ifstream input(argv[2]);std::ofstream output(argv[3]);require(input.good() && output.good());
        output<<std::setprecision(17);
        output<<"time,variant,wheel,target,raw_target,target_rate,measurement,measured_rate,y,v,load,dt,"
                 "td1,td2,z1,z2,z3,request,applied,actuator,next_td1,next_td2,next_z1,next_z2,next_z3,"
                 "next_y,next_v,next_actuator,power,scale,handover_error\n";
        std::array<std::array<Node,4>,4> nodes;
        for(int variant=0;variant<p.variants;++variant) for(int j=0;j<p.wheels;++j) nodes[variant][j].init(p);
        std::string line;std::getline(input,line);int rows=0;
        while(std::getline(input,line)) {
            if(line.empty()) continue;
            std::replace(line.begin(),line.end(),',',' ');std::istringstream parser(line);
            std::array<double,18> row{};for(double& value:row) {require(static_cast<bool>(parser>>value));require(std::isfinite(value));}
            std::string extra;require(!(parser>>extra));
            const double time=row[0];const float dt=static_cast<float>(row[17]);require(dt>0 && dt<0.01f);
            for(int variant=0;variant<p.variants;++variant) {
                auto& group=nodes[variant];
                std::array<float,4> target{},rate{},raw_target{},measurement{},measured_rate{},request{},applied{},error{},candidate_integral{},handover{};
                std::array<std::array<float,3>,4> before{};
                for(int j=0;j<p.wheels;++j) {
                    Node& n=group[j];raw_target[j]=static_cast<float>(row[1+j]);target[j]=raw_target[j];
                    rate[j]=static_cast<float>(row[5+j]);measurement[j]=static_cast<float>(n.y+row[9+j]);
                    measured_rate[j]=static_cast<float>(n.v);
                    if(id==2) {
                        const float continuous=n.target_stream.update(raw_target[j]);
                        (void)continuous; // 本拍只更新目标展开状态；TD在本区间结束后推进。
                        measurement[j]=n.measurement_stream.update(static_cast<float>(wrap(measurement[j])));
                        target[j]=n.td.GetOutput();rate[j]=n.td.GetRate();
                    }
                    const bool pid=(variant==0 || (id==3 && variant==2 && time<2.0));
                    if(id==3 && variant==2 && rows==2000) {
                        const float initial_f=p.wc*p.wc*(target[j]-measurement[j])+
                            2*p.wc*(rate[j]-measured_rate[j])-p.b0*n.last_applied;
                        require(n.angle.Reset(measurement[j],measured_rate[j],initial_f));
                    }
                    before[j]=state(n,id,variant);
                    error[j]=target[j]-measurement[j];
                    candidate_integral[j]=n.integral+dt*error[j];
                    if(pid) {
                        if(id==1 || id==4) request[j]=(p.wc/p.b0)*error[j]+static_cast<float>(p.wc*p.damping/p.b0)*candidate_integral[j];
                        else request[j]=(p.wc*p.wc/p.b0)*error[j]+(2*p.wc/p.b0)*(rate[j]-measured_rate[j])+
                            (0.15f*p.wc*p.wc*p.wc/p.b0)*candidate_integral[j];
                    } else if(id==1 && variant==2) require(n.nlesf1.Calc(target[j],n.eso2.GetOutput(),n.eso2.GetDisturbance(),request[j]));
                    else if(id==1 || id==4) require(n.speed.Calc(target[j],request[j]));
                    else if(id==3 && variant==3) require(n.nlesf2.Calc(target[j],rate[j],n.eso3.GetOutput(),n.eso3.GetRate(),n.eso3.GetDisturbance(),request[j]));
                    else if((id==2 || id==3) && variant==1) require(n.angle.Calc(target[j],request[j]));
                    else require(n.angle.Calc(target[j],rate[j],request[j]));
                    require(std::isfinite(request[j]));applied[j]=clip(request[j],p.limit);
                }
                const float scale=id==4?power_scale(applied,group):1.0f;
                double power=0;
                for(int j=0;j<p.wheels;++j) {applied[j]*=scale;power+=2.0*applied[j]*applied[j]+0.4*std::fabs(applied[j]*group[j].y);}
                for(int j=0;j<p.wheels;++j) {
                    Node& n=group[j];const bool pid=(variant==0 || (id==3 && variant==2 && time<2.0));
                    if(pid && (std::fabs(request[j]-applied[j])<1e-6f || error[j]*(request[j]-applied[j])<=0)) n.integral=candidate_integral[j];
                    if(id==3 && variant==2 && rows==2000) handover[j]=request[j]-n.last_applied;
                    const float td1=n.td.GetOutput(),td2=n.td.GetRate();
                    // 本拍控制完成，最终输出处理后才预测下一拍估计。
                    if(variant!=0 && !pid) {
                        if(id==1 && variant==2) require(n.eso2.Update(applied[j],measurement[j],dt));
                        else if(id==1 || id==4) require(n.speed.UpdateObserver(id==4 && variant==2?request[j]:applied[j],measurement[j],dt));
                        else if(id==3 && variant==3) require(n.eso3.Update(applied[j],measurement[j],dt));
                        else require(n.angle.UpdateObserver(applied[j],measurement[j],dt));
                    }
                    if(id==2) require(n.td.Update(n.target_stream.lifted,dt));
                    const auto after=state(n,id,variant);
                    // 双精度模拟对象，包含两拍命令延迟和电流响应；不是库核心。
                    const double old_y=n.y,old_v=n.v,old_actuator=n.actuator;
                    const double delayed=n.delayed[0];n.delayed[0]=n.delayed[1];n.delayed[1]=applied[j];
                    n.actuator+=(1-std::exp(-double(dt)/p.tau))*(delayed-n.actuator);
                    const double load=row[13+j]-(id==3?4.0*std::cos(old_y):0.0);
                    const double forcing=p.b0*n.actuator+load;
                    const double decay=std::exp(-p.damping*double(dt)),gain=(1-decay)/p.damping;
                    if(id==1 || id==4) {n.y=decay*old_y+gain*forcing;n.v=0;}
                    else {n.y=old_y+gain*old_v+(double(dt)-gain)*forcing/p.damping;n.v=decay*old_v+gain*forcing;}
                    require(std::isfinite(n.y) && std::isfinite(n.v));
                    output<<time<<','<<variant<<','<<j<<','<<target[j]<<','<<raw_target[j]<<','<<rate[j]<<','<<measurement[j]<<','<<measured_rate[j]<<','
                        <<old_y<<','<<old_v<<','<<load<<','<<dt<<','<<td1<<','<<td2<<','<<before[j][0]<<','<<before[j][1]<<','<<before[j][2]<<','
                        <<request[j]<<','<<applied[j]<<','<<old_actuator<<','<<n.td.GetOutput()<<','<<n.td.GetRate()<<','
                        <<after[0]<<','<<after[1]<<','<<after[2]<<','<<n.y<<','<<n.v<<','<<n.actuator<<','<<power<<','<<scale<<','<<handover[j]<<'\n';
                    n.last_applied=applied[j];
                }
            }
            ++rows;
        }
        require(rows>0 && output.good());std::cout<<"APPLICATION_PASS id="<<id<<" fixture_rows="<<rows<<'\n';
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
