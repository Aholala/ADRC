function T = adrc_final_closed(p,time,target,force,noise,limit,precision,reference,replay_measurements)
% 第28/29讲共享教学对象。reference=true时直接调用原三个模块；不属于C++生产层。
if nargin<9, replay_measurements=[]; end
c=cast(p.eso,precision); f=cast(p.feedback,precision); h=cast(p.h,precision);
r=cast(p.td_r,precision); h0=cast(p.td_h0,precision);
z=zeros(3,1,precision); x=zeros(2,1,precision); y=0;v=0;
records=zeros(numel(time),22);
for k=1:numel(time)
    measurement=cast(y+noise(k),precision);
    if ~isempty(replay_measurements), measurement=cast(replay_measurements(k),precision); end
    tk=cast(target(k),precision);
    if reference
        u0=NLSEF_2fal(time(k),[],[x(1)-z(1);x(2)-z(2)],3,f(2),f(3),f(4),f(5),f(6));
        requested=(u0-z(3))/c(1);
    else
        requested=adrc_sef(x(1),x(2),z,f);
    end
    applied=requested;
    if limit>0, applied=max(-cast(limit,precision),min(cast(limit,precision),requested)); end
    if reference
        dz=ESO2_NL(time(k),z,[measurement;applied],1,c(5),c(6),c(7),c(2:4),c(1));
        zn=z+h*dz(:);
        xn=TD2_NL(time(k),x,tk,2,r,h,h0);xn=xn(:);
    else
        zn=adrc_eso3_step(z,applied,measurement,c,h);
        xn=td_step(x,tk,r,h,h0);
    end
    acceleration=double(c(1))*double(applied)+double(force(k));
    yn=y+double(h)*v+0.5*double(h)^2*acceleration; vn=v+double(h)*acceleration;
    records(k,:)=[time(k),double(tk),double(force(k)),noise(k),double(h),double(measurement),y,v, ...
        double(x'),double(z'),double(requested),double(applied),double(xn'),double(zn'),yn,vn];
    z=zn;x=xn;y=yn;v=vn;
end
assert(all(isfinite(records),'all'),'ADRC:NonfiniteClosedLoop','完整闭环存在非有限数值。');
T=array2table(records,'VariableNames',{'time','target','force','noise','dt','measurement','true_y','true_rate', ...
    'td1','td2','z1','z2','z3','requested','applied','next_td1','next_td2','next_z1','next_z2','next_z3','next_y','next_rate'});
end
