library(tidyverse)

f_mm <- 800
F_ratio <- 4
D_pm_mm <- f_mm / F_ratio
a_pm <- 1 / 4 / f_mm

calc_z_pm_mm <- function(a, x, y) {
  r2 <- x * x + y * y
  z <- a * r2
  return(z)
}

x_pm_mm <- seq(-D_pm_mm / 2, +D_pm_mm / 2, 1)
#z_pm_mm <- pm_a * x_pm * x_pm
pm <- data.frame(x = x_pm, z = calc_z_pm_mm(a_pm, x_pm_mm, 0.0))

x0_sm_mm <- 0
z0_sm_mm <- 600
D_sm_mm <- 100
theta_sm_deg <- 45
theta_sm_rad <- theta_sm_deg * pi / 180
sm <- data.frame(x = c(x0_sm_mm - D_sm_mm / 2 * cos(theta_sm_rad), x0_sm_mm + D_sm_mm / 2 * cos(theta_sm_rad)),
                 z = c(z0_sm_mm - D_sm_mm / 2 * sin(theta_sm_rad), z0_sm_mm + D_sm_mm / 2 * sin(theta_sm_rad)))

L_tube_mm <- 700
D_tube_mm <- 232; hhh <- D_tube_mm / 2
tube <- data.frame(x = c(-hhh, -hhh, +hhh, + hhh), z = c(L_tube_mm, 0, 0, L_tube_mm))

calc_xyz0 <- function(xyz1) {
  with(xyz1,{
    x0_mm <- x1_mm
    y0_mm <- y1_mm
    z0_mm <- L_tube_mm + 25
    xyz0 <<- data.frame(r = r, i = 0, x_mm = x0_mm, y_mm = y0_mm, z_mm = z0_mm,
                       u_deg = u_deg, v_deg = v_deg, Vig = FALSE)
  })
  return(xyz0)
}
calc_xyz2 <- function(xyz1) {
  #z = 2*a*x
  x2_mm <- 
  
  with(xyz1,{
    x2_mm <- x1_mm
    y2_mm <- y1_mm
    z2_mm <- L_tube_mm + 25
    xyz0 <<- data.frame(r = r, i = 0, x_mm = x0_mm, y_mm = y0_mm, z_mm = z0_mm,
                        u_deg = u_deg, v_deg = v_deg, Vig = FALSE)
  })
  return(xyz0)
}

+ 主鏡上の位置と主鏡入射光の方向余弦を与える
+ 遡って鏡筒+25mmの位置を計算 -> xyzlmn
    + 鏡筒先端でケラれる場合はVig = TRUE
+ 主鏡反射光の方向余弦を計算
+ 副鏡入射位置を計算
+ 副鏡でケラれる場合、Vig=TRUE
+ 副鏡反射光の方向余弦を計算
+ 像面/でフォーカス位置での光線位置を計算

反射計算のけんしょうからかな


ray <- data.frame()
r <- 1
for(x1_mm in c(-100, -50, 0, 50, 100)) {
  #x1_mm <- -100
  y1_mm <- 0
  z1_mm <- calc_z_pm_mm(a_pm, x1_mm, y1_mm)
  u_deg <- 0
  v_deg <- 0
  xyz1 <- data.frame(r = r, i = 1, x_mm = x1_mm, y_mm = y1_mm, z_mm = z1_mm,
                     u_deg = u_deg, v_deg = v_deg, Vig = FALSE)
  xyz0 <- calc_xyz0(xyz1)
  dat_xyz <- bind_rows(xyz0, xyz1)
  ray <- bind_rows(ray, dat_xyz)
  r <- r + 1
}

g <- ggplot()
g <- g + geom_path(data = tube, aes(x, z), color = "gray", size = 1)
g <- g + geom_line(data = pm, aes(x, z), color = "blue", size = 1)
g <- g + geom_line(data = sm, aes(x, z), color = "green", size = 1)
g <- g + geom_path(data = ray, aes(x_mm, z_mm, group = r), color = "red")
g <- g + coord_equal(xlim = c(-110, 200), ylim = c(0, 725))
g <- g + coord_cartesian(xlim = c(-110, 200), ylim = c(0, 20))
plot(g)