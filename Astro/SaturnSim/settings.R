default_camera <- "RGGB"
# interpolations <- c("なし",
#                     "隣接画素縮小補間",
#                     "隣接画素補間",
#                     "重心一致縮小補間",
#                     "重心一致補間")
interpolations <- c("なし", 
                    "隣接画素縮小補間", 
                    "隣接画素補間", 
                    #"重心一致縮小補間", 
                    "重心一致補間")
#default_interpolation <- "隣接画素縮小補間"
default_interpolation <- "隣接画素縮小補間"
default_interpolation <- "隣接画素補間"
default_interpolation <- "重心一致補間"
#0:元画像のスケール

#img0 <- load.image("D:/astrophoto/sorabiz/image/Saturn_HST_2004-03-22.jpg")
img0 <- load.image("/ws/Astro/ASApp/img//Saturn_HST_2004-03-22.jpg")
img0 <- (img0 / max(img0)) ^ (2.2)
pix0_arcsec <- 19.43 / 890
#1:シミュレーション分解能のスケール
#（無し）:結果画像データのスケール
