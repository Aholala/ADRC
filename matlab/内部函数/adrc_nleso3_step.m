function next = adrc_nleso3_step(z,input,measurement,c,h)
% 第26讲：原ESO2_NL连续导数明确采用前向欧拉，首行线性、后两行fal。
assert(isequal(size(z),[3 1]) && all(isfinite(z)) && isfinite(input) && isfinite(measurement) ...
    && isfinite(h) && h>0 && all(isfinite(c)) && numel(c)==7 && c(1)~=0 ...
    && all(c(2:4)>0) && all(c(5:6)>0 & c(5:6)<=1) && c(7)>0,'ADRC:InvalidNLESO','检查状态、配置和周期。');
e=z(1)-measurement;
f1=adrc_fal(e,c(5),c(7)); f2=adrc_fal(e,c(6),c(7));
next=[z(1)+h*(z(2)-c(2)*e);z(2)+h*(z(3)-c(3)*f1+c(1)*input);z(3)+h*(-c(4)*f2)];
assert(all(isfinite(next)),'ADRC:NonfiniteNLESO','非线性ESO得到非有限状态。');
end
