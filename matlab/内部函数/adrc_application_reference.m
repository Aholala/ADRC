function T=adrc_application_reference(p,F)
% 独立MATLAB闭环：单精度控制算法、双精度模拟对象，不读取C++对象或测量。
data=F{:,:};V=p.variants;W=p.wheels;N=size(data,1);h=single(p.h);b=single(p.b0);wc=single(p.wc);wo=single(p.wo);
y=p.initial*ones(V,W);v=zeros(V,W);act=zeros(V,W);delay=zeros(V,W,2);
z=zeros(V,W,3,'single');z(:,:,1)=single(p.initial);td=zeros(V,W,2,'single');td(:,:,1)=single(p.initial);
integral=zeros(V,W,'single');last_applied=zeros(V,W,'single');
last_target=single(wrap_angle(p.initial))*ones(V,W,'single');lifted_target=single(p.initial)*ones(V,W,'single');
last_meas=last_target;lifted_meas=lifted_target;
records=zeros(N*V*W,31);
for k=1:N
    time=data(k,1);
    for variant=0:V-1
        a=variant+1;
        raw=single(data(k,2:1+W));r=raw;rd=single(data(k,6:5+W));
        measurement=single(y(a,:)+data(k,10:9+W));measured_rate=single(v(a,:));
        if p.id==2
            for j=1:W
                lifted_target(a,j)=lifted_target(a,j)+single(wrap_delta(raw(j)-last_target(a,j)));last_target(a,j)=raw(j);
                wrapped=single(wrap_angle(double(measurement(j))));
                lifted_meas(a,j)=lifted_meas(a,j)+single(wrap_delta(wrapped-last_meas(a,j)));last_meas(a,j)=wrapped;
            end
            measurement=lifted_meas(a,:);r=td(a,:,1);rd=td(a,:,2);
        end
        pid=variant==0 || (p.id==3 && variant==2 && time<2);
        if p.id==3 && variant==2 && k==2001
            z(a,:,1)=measurement;z(a,:,2)=measured_rate;
            z(a,:,3)=wc*wc*(r-measurement)+single(2)*wc*(rd-measured_rate)-b*last_applied(a,:);
        end
        before=reshape(z(a,:,:),W,3);oldtd=reshape(td(a,:,:),W,2);
        error=r-measurement;candidate=integral(a,:)+h*error;request=zeros(1,W,'single');
        for j=1:W
            state=reshape(z(a,j,:),3,1);
            if pid
                if p.id==1 || p.id==4
                    request(j)=(wc/b)*error(j)+single(double(wc)*p.damping/double(b))*candidate(j);
                else
                    request(j)=(wc*wc/b)*error(j)+(single(2)*wc/b)*(rd(j)-measured_rate(j))+(single(0.15)*wc*wc*wc/b)*candidate(j);
                end
            elseif p.id==1 && variant==2
                request(j)=adrc_nlesf1(r(j),state(1),state(3),p.nlesf1);
            elseif p.id==1 || p.id==4
                request(j)=adrc_control(r(j),state([1 3]),b,wc);
            elseif p.id==3 && variant==3
                request(j)=adrc_sef(r(j),rd(j),state,p.nlesf2);
            elseif (p.id==2 || p.id==3) && variant==1
                request(j)=adrc_control2(r(j),state,b,wc);
            else
                request(j)=(wc*wc*(r(j)-state(1))+single(2)*wc*(rd(j)-state(2))-state(3))/b;
            end
        end
        applied=max(-single(p.limit),min(single(p.limit),request));scale=single(1);
        if p.id==4
            copper=sum(2*double(applied).^2);mechanical=sum(0.4*abs(double(applied).*y(a,:)));
            if copper+mechanical>p.power_limit,scale=single(min(1,2*p.power_limit/(mechanical+sqrt(mechanical^2+4*copper*p.power_limit))));end
        end
        applied=applied*scale;power=sum(2*double(applied).^2+0.4*abs(double(applied).*y(a,:)));
        for j=1:W
            handover=single(0);
            if pid && (abs(request(j)-applied(j))<single(1e-6) || error(j)*(request(j)-applied(j))<=0),integral(a,j)=candidate(j);end
            if p.id==3 && variant==2 && k==2001,handover=request(j)-last_applied(a,j);end
            state=reshape(z(a,j,:),3,1);
            if variant~=0 && ~pid
                if p.id==1 && variant==2
                    next=adrc_eso2_step(state([1 3]),applied(j),measurement(j),p.eso2,h);state=[next(1);single(0);next(2)];
                elseif p.id==1 || p.id==4
                    observer_input=applied(j);if p.id==4 && variant==2,observer_input=request(j);end
                    next=adrc_eso_step(state([1 3]),observer_input,measurement(j),b,single(2)*wo,wo*wo,h);state=[next(1);single(0);next(2)];
                elseif p.id==3 && variant==3
                    state=adrc_eso3_step(state,applied(j),measurement(j),p.eso3,h);
                else
                    state=adrc_leso3_step(state,applied(j),measurement(j),b,single(3)*wo,single(3)*wo*wo,wo*wo*wo,h);
                end
            end
            z(a,j,:)=state;
            if p.id==2
                nexttd=ltd_step(reshape(td(a,j,:),2,1),lifted_target(a,j),single(12),h);td(a,j,:)=nexttd;
            end
            old_y=y(a,j);old_v=v(a,j);old_act=act(a,j);
            delayed=delay(a,j,1);delay(a,j,1)=delay(a,j,2);delay(a,j,2)=double(applied(j));
            act(a,j)=act(a,j)+(1-exp(-double(h)/p.tau))*(delayed-act(a,j));
            load=data(k,13+j);if p.id==3,load=load-4*cos(old_y);end
            forcing=double(b)*act(a,j)+load;decay=exp(-p.damping*double(h));gain=(1-decay)/p.damping;
            if p.id==1 || p.id==4,y(a,j)=decay*old_y+gain*forcing;v(a,j)=0;
            else,y(a,j)=old_y+gain*old_v+(double(h)-gain)*forcing/p.damping;v(a,j)=decay*old_v+gain*forcing;end
            index=((k-1)*V+variant)*W+j;
            records(index,:)=[time,variant,j-1,double(r(j)),double(raw(j)),double(rd(j)),double(measurement(j)),double(measured_rate(j)), ...
                old_y,old_v,load,double(h),double(oldtd(j,:)),double(before(j,:)),double(request(j)),double(applied(j)),old_act, ...
                double(reshape(td(a,j,:),1,2)),double(state'),y(a,j),v(a,j),act(a,j),power,double(scale),double(handover)];
            last_applied(a,j)=applied(j);
        end
    end
end
assert(all(isfinite(records),'all'));
names={'time','variant','wheel','target','raw_target','target_rate','measurement','measured_rate','y','v','load','dt', ...
    'td1','td2','z1','z2','z3','request','applied','actuator','next_td1','next_td2','next_z1','next_z2','next_z3','next_y','next_v','next_actuator','power','scale','handover_error'};
T=array2table(records,'VariableNames',names);
end
function x=wrap_angle(x)
x=x-2*pi*floor((x+pi)/(2*pi));
end
function x=wrap_delta(x)
while x>single(pi),x=x-single(2*pi);end
while x<-single(pi),x=x+single(2*pi);end
end
