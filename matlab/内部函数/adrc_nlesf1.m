function command=adrc_nlesf1(target,output,disturbance,c)
% 单误差非线性反馈：c=[b0,beta,alpha,delta]，无动态状态。
assert(numel(c)==4 && all(isfinite([target;output;disturbance;c(:)])) ...
    && c(1)~=0 && all(c(2:4)>0));
command=(c(2)*adrc_fal(target-output,c(3),c(4))-disturbance)/c(1);
assert(isfinite(command));
end
