function next=adrc_eso2_step(z,input,measurement,c,h)
% 一阶扩张模型的补充实现：c=[b0,beta1,beta2,alpha,delta]。
% z1估计输出，z2估计总扰动；两个右侧均使用旧状态。
assert(numel(z)==2 && numel(c)==5 && all(isfinite([z(:);c(:);input;measurement;h])) ...
    && c(1)~=0 && all(c(2:3)>0) && c(4)>0 && c(4)<=1 && c(5)>0 && h>0);
e=z(1)-measurement;correction=adrc_fal(e,c(4),c(5));
next=[z(1)+h*(z(2)+c(1)*input-c(2)*e);z(2)-h*c(3)*correction];
assert(all(isfinite(next)));
end
