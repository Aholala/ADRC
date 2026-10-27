function least_square_method()
% 最小二乘拟合功率模型：P = k0 + k1*T + k2*w + k3*T*w + k4*T^2 + k5*w^2
% 从 3508_power.csv 读取数据（列顺序：P、T、w）

    data = readmatrix('M3508.csv');
    power = data(:, 1);
    T = data(:, 2);
    w = data(:, 3);

    x = [T, w];
    k0 = [1 1 1 1 1 1];

    % 最小二乘法说明：
    % 1) 定义残差 r_i = P_i(实测) - P_i(模型)
    % 2) 目标是让所有残差的平方和最小：min_k  sum_i r_i^2
    % 3) 这样可以在整体意义上最“贴近”数据，减少大误差的影响
    % 4) 这里用 k = [k0 k1 k2 k3 k4 k5] 拟合二次模型的系数
    model = @(k, xdata) k(1) + k(2).*xdata(:,1) + k(3).*xdata(:,2) + ...
        k(4).*xdata(:,1).*xdata(:,2) + k(5).*xdata(:,1).^2 + k(6).*xdata(:,2).^2;

    if exist('lsqcurvefit', 'file') == 2
        opts = optimoptions('lsqcurvefit', 'Display', 'off');
        k = lsqcurvefit(model, k0, x, power, [], [], opts);
    elseif exist('nlinfit', 'file') == 2
        k = nlinfit(x, power, model, k0);
    else
        % 备用方案：直接最小化最小二乘目标函数。
        obj = @(k) sum((model(k, x) - power).^2);
        k = fminsearch(obj, k0);
    end

    fprintf('k0: %.12g\n', k(1));
    fprintf('k1: %.12g\n', k(2));
    fprintf('k2: %.12g\n', k(3));
    fprintf('k3: %.12g\n', k(4));
    fprintf('k4: %.12g\n', k(5));
    fprintf('k5: %.12g\n', k(6));

    w_fixed = 300;
    T_min = min(T);
    T_max = max(T);
    T_grid = linspace(T_min, T_max, 300)';
    x_grid = [T_grid, w_fixed * ones(size(T_grid))];
    P_grid = model(k, x_grid);
    figure;
    plot(T_grid, P_grid, 'LineWidth', 1.5);
    grid on;
    xlabel('T');
    ylabel('P');
    title('Power curve at w = 200 rad/s');
end

