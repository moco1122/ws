library(shiny)
library(tidyverse)
library(glue)
library(imager)

#処理スルー用のダミーPSFを作成して返す
create_psf_unity <- function(N_pix) {
  psf <- as.cimg(function(x, y) { x * 0.0 }, N_pix, N_pix)
  psf[N_pix / 2, N_pix / 2, ,] <- 1
  psf
}

#N_pix * N_pixサイズで幅sigma_seeing_pixのシーイングを表すPSFを作成して返す
create_psf_gaussian <- function(N_pix, sigma_pix) {
  if(sigma_pix <= 0.0) { 
    #psf <- create_psf_unity(N_pix) 
    psf <- create_psf_aperture(N_pix, 1) 
  }
  else {
    psf <- as.cimg(function(x, y) {
      exp(-((x - N_pix / 2)^2 + (y - N_pix / 2)^2)/(2 * sigma_pix^2))}, N_pix, N_pix)
  }
  
  psf
}

create_psf_airydisc <- function(N_pix, f_ratio, pix_um, lambda_um) {
  psf <- as.cimg(function(x, y) {
    r_um <- sqrt((x - N_pix / 2)^2 + (y - N_pix / 2)^2) * pix_um; 
    xx <- pi * r_um / lambda_um / f_ratio;
    (2 * besselJ( xx, 1) / xx)^2  }, N_pix, N_pix)
  psf[N_pix / 2, N_pix / 2, ,] <- 1
  
  psf
}

create_psf_aperture <- function(N_pix, sim_resolution) {
  psf <- as.cimg(function(x, y) { x * 0.0 }, N_pix, N_pix)
  psf[(N_pix / 2 - sim_resolution / 2):(N_pix / 2 + sim_resolution / 2 - 1), 
      (N_pix / 2 - sim_resolution / 2):(N_pix / 2 + sim_resolution / 2 - 1), ,] <- 1
  psf
}

#RGB画像を3バンドべイヤー配列画像に変換
bayerize <- function(img) {
  dim_img <- dim(img)
  w0 <- dim_img[1]; w <- ceiling(w0 / 2) * 2
  h0 <- dim_img[2]; h <- ceiling(h0 / 2) * 2
  bayer <- cimg(array(0.0, c(w, h, 1, 3)))
  
  idx0 <- seq(1, floor(w0 / 2) * 2, 2)
  idy0 <- seq(1, floor(h0 / 2) * 2, 2)
  
  bayer[idx0    , idy0    , , 1] <- img[idx0    , idy0    , , 1]
  bayer[idx0 + 1, idy0    , , 2] <- img[idx0 + 1, idy0    , , 2]
  bayer[idx0    , idy0 + 1, , 2] <- img[idx0    , idy0 + 1, , 2]
  bayer[idx0 + 1, idy0 + 1, , 3] <- img[idx0 + 1, idy0 + 1, , 3]
  
  bayer
}

#最近傍画素による簡易縮小デモザイク
demosaic_nearest_downsampling <- function(bayer) {
  dim_img <- dim(bayer)
  w0 <- dim_img[1]; w <- ceiling(w0 / 2) * 2
  h0 <- dim_img[2]; h <- ceiling(h0 / 2) * 2
  demosaiced <- imresize(bayer, 0.5, -1) * 0.0

  idx0 <- seq(1, w0, 2)
  idy0 <- seq(1, h0, 2)
  idx <- 1:(w / 2)
  idy <- 1:(h / 2); 
  
  idc <- 1 #R
  demosaiced[idx    , idy    , , idc] <- bayer[idx0, idy0, , idc] 
  idc <- 2 #G
  demosaiced[idx    , idy    , , idc] <- (bayer[idx0 + 1, idy0, , idc] + bayer[idx0    , idy0 + 1, , idc]) / 2
  idc <- 3 #B
  demosaiced[idx    , idy    , , idc] <- bayer[idx0 + 1, idy0 + 1, , idc]
  
  demosaiced
}

#隣接画素補間によるデモザイク
demosaic_nearest <- function(bayer) {
  dim_img <- dim(bayer)
  w0 <- dim_img[1]; w <- floor(w0 / 2) * 2
  h0 <- dim_img[2]; h <- floor(h0 / 2) * 2
  demosaiced <- bayer * 0.0
  
  idx <- seq(3, w - 2, 2)
  idy <- seq(3, h - 2, 2)
  idc <- 1 #R
  demosaiced[idx    , idy    , , idc] <- bayer[idx, idy, , idc] 
  demosaiced[idx + 1, idy    , , idc] <- (bayer[idx, idy, , idc] + bayer[idx + 2, idy, , idc]) / 2
  demosaiced[idx    , idy + 1, , idc] <- (bayer[idx, idy, , idc] + bayer[idx, idy + 2, , idc]) / 2
  demosaiced[idx + 1, idy + 1, , idc] <- (bayer[idx, idy, , idc] + bayer[idx + 2, idy, , idc] + bayer[idx, idy + 2, , idc] + bayer[idx + 2, idy + 2, , idc]) / 4
  idc <- 2 #G
  demosaiced[idx    , idy    , , idc] <- (bayer[idx - 1, idy, , idc] + bayer[idx + 1, idy, , idc] + bayer[idx, idy - 1, , idc] + bayer[idx, idy + 1, , idc]) / 4
  demosaiced[idx + 1, idy    , , idc] <- bayer[idx + 1, idy, , idc]
  demosaiced[idx    , idy + 1, , idc] <- bayer[idx, idy + 1, , idc]
  demosaiced[idx + 1, idy + 1, , idc] <- (bayer[idx, idy + 1, , idc] + bayer[idx + 2, idy + 1, , idc] + bayer[idx + 1, idy, , idc] + bayer[idx + 1, idy + 2, , idc]) / 4
  idc <- 3 #B
  demosaiced[idx    , idy    , , idc] <- (bayer[idx - 1, idy - 1, , idc] + bayer[idx + 1, idy - 1, , idc] + bayer[idx - 1, idy + 1, , idc] + bayer[idx + 1, idy + 1, , idc]) / 4
  demosaiced[idx + 1, idy    , , idc] <- (bayer[idx + 1, idy - 1, , idc] + bayer[idx + 1, idy + 1, , idc]) / 2
  demosaiced[idx    , idy + 1, , idc] <- (bayer[idx - 1, idy + 1, , idc] + bayer[idx + 1, idy + 1, , idc]) / 2
  demosaiced[idx + 1, idy + 1, , idc] <- bayer[idx + 1, idy + 1, , idc]
  
  idx <- 1
  idy <- seq(3, h - 2, 2)
  idc <- 1 #R
  demosaiced[idx    , idy    , , idc] <- bayer[idx, idy, , idc] 
  demosaiced[idx + 1, idy    , , idc] <- (bayer[idx, idy, , idc] + bayer[idx + 2, idy, , idc]) / 2
  demosaiced[idx    , idy + 1, , idc] <- (bayer[idx, idy, , idc] + bayer[idx, idy + 2, , idc]) / 2
  demosaiced[idx + 1, idy + 1, , idc] <- (bayer[idx, idy, , idc] + bayer[idx + 2, idy, , idc] + bayer[idx, idy + 2, , idc] + bayer[idx + 2, idy + 2, , idc]) / 4
  idc <- 2 #G
  demosaiced[idx    , idy    , , idc] <- (bayer[idx + 1, idy, , idc] + bayer[idx + 1, idy, , idc] + bayer[idx, idy - 1, , idc] + bayer[idx, idy + 1, , idc]) / 4
  demosaiced[idx + 1, idy    , , idc] <- bayer[idx + 1, idy, , idc]
  demosaiced[idx    , idy + 1, , idc] <- bayer[idx, idy + 1, , idc]
  demosaiced[idx + 1, idy + 1, , idc] <- (bayer[idx, idy + 1, , idc] + bayer[idx + 2, idy + 1, , idc] + bayer[idx + 1, idy, , idc] + bayer[idx + 1, idy + 2, , idc]) / 4
  idc <- 3 #B
  demosaiced[idx    , idy    , , idc] <- (bayer[idx + 1, idy - 1, , idc] + bayer[idx + 1, idy - 1, , idc] + bayer[idx + 1, idy + 1, , idc] + bayer[idx + 1, idy + 1, , idc]) / 4
  demosaiced[idx + 1, idy    , , idc] <- (bayer[idx + 1, idy - 1, , idc] + bayer[idx + 1, idy + 1, , idc]) / 2
  demosaiced[idx    , idy + 1, , idc] <- (bayer[idx + 1, idy + 1, , idc] + bayer[idx + 1, idy + 1, , idc]) / 2
  demosaiced[idx + 1, idy + 1, , idc] <- bayer[idx + 1, idy + 1, , idc]

  idx <- w - 1
  idy <- seq(3, h - 2, 2)
  idc <- 1 #R
  demosaiced[idx    , idy    , , idc] <- bayer[idx, idy, , idc] 
  demosaiced[idx + 1, idy    , , idc] <- (bayer[idx, idy, , idc] + bayer[idx, idy, , idc]) / 2
  demosaiced[idx    , idy + 1, , idc] <- (bayer[idx, idy, , idc] + bayer[idx, idy + 2, , idc]) / 2
  demosaiced[idx + 1, idy + 1, , idc] <- (bayer[idx, idy, , idc] + bayer[idx, idy, , idc] + bayer[idx, idy + 2, , idc] + bayer[idx, idy + 2, , idc]) / 4
  idc <- 2 #G
  demosaiced[idx    , idy    , , idc] <- (bayer[idx - 1, idy, , idc] + bayer[idx + 1, idy, , idc] + bayer[idx, idy - 1, , idc] + bayer[idx, idy + 1, , idc]) / 4
  demosaiced[idx + 1, idy    , , idc] <- bayer[idx + 1, idy, , idc]
  demosaiced[idx    , idy + 1, , idc] <- bayer[idx, idy + 1, , idc]
  demosaiced[idx + 1, idy + 1, , idc] <- (bayer[idx, idy + 1, , idc] + bayer[idx, idy + 1, , idc] + bayer[idx + 1, idy, , idc] + bayer[idx + 1, idy + 2, , idc]) / 4
  idc <- 3 #B
  demosaiced[idx    , idy    , , idc] <- (bayer[idx - 1, idy - 1, , idc] + bayer[idx + 1, idy - 1, , idc] + bayer[idx - 1, idy + 1, , idc] + bayer[idx + 1, idy + 1, , idc]) / 4
  demosaiced[idx + 1, idy    , , idc] <- (bayer[idx + 1, idy - 1, , idc] + bayer[idx + 1, idy + 1, , idc]) / 2
  demosaiced[idx    , idy + 1, , idc] <- (bayer[idx - 1, idy + 1, , idc] + bayer[idx + 1, idy + 1, , idc]) / 2
  demosaiced[idx + 1, idy + 1, , idc] <- bayer[idx + 1, idy + 1, , idc]
  
  idx <- seq(3, w - 2, 2)
  idy <- 1
  idc <- 1 #R
  demosaiced[idx    , idy    , , idc] <- bayer[idx, idy, , idc] 
  demosaiced[idx + 1, idy    , , idc] <- (bayer[idx, idy, , idc] + bayer[idx + 2, idy, , idc]) / 2
  demosaiced[idx    , idy + 1, , idc] <- (bayer[idx, idy, , idc] + bayer[idx, idy + 2, , idc]) / 2
  demosaiced[idx + 1, idy + 1, , idc] <- (bayer[idx, idy, , idc] + bayer[idx + 2, idy, , idc] + bayer[idx, idy + 2, , idc] + bayer[idx + 2, idy + 2, , idc]) / 4
  idc <- 2 #G
  demosaiced[idx    , idy    , , idc] <- (bayer[idx - 1, idy, , idc] + bayer[idx + 1, idy, , idc] + bayer[idx, idy + 1, , idc] + bayer[idx, idy + 1, , idc]) / 4
  demosaiced[idx + 1, idy    , , idc] <- bayer[idx + 1, idy, , idc]
  demosaiced[idx    , idy + 1, , idc] <- bayer[idx, idy + 1, , idc]
  demosaiced[idx + 1, idy + 1, , idc] <- (bayer[idx, idy + 1, , idc] + bayer[idx + 2, idy + 1, , idc] + bayer[idx + 1, idy, , idc] + bayer[idx + 1, idy + 2, , idc]) / 4
  idc <- 3 #B
  demosaiced[idx    , idy    , , idc] <- (bayer[idx - 1, idy + 1, , idc] + bayer[idx + 1, idy + 1, , idc] + bayer[idx - 1, idy + 1, , idc] + bayer[idx + 1, idy + 1, , idc]) / 4
  demosaiced[idx + 1, idy    , , idc] <- (bayer[idx + 1, idy + 1, , idc] + bayer[idx + 1, idy + 1, , idc]) / 2
  demosaiced[idx    , idy + 1, , idc] <- (bayer[idx - 1, idy + 1, , idc] + bayer[idx + 1, idy + 1, , idc]) / 2
  demosaiced[idx + 1, idy + 1, , idc] <- bayer[idx + 1, idy + 1, , idc]
  
  idx <- seq(3, w - 2, 2)
  idy <- h - 1
  idc <- 1 #R
  demosaiced[idx    , idy    , , idc] <- bayer[idx, idy, , idc] 
  demosaiced[idx + 1, idy    , , idc] <- (bayer[idx, idy, , idc] + bayer[idx + 2, idy, , idc]) / 2
  demosaiced[idx    , idy + 1, , idc] <- (bayer[idx, idy, , idc] + bayer[idx, idy, , idc]) / 2
  demosaiced[idx + 1, idy + 1, , idc] <- (bayer[idx, idy, , idc] + bayer[idx + 2, idy, , idc] + bayer[idx, idy, , idc] + bayer[idx + 2, idy, , idc]) / 4
  idc <- 2 #G
  demosaiced[idx    , idy    , , idc] <- (bayer[idx - 1, idy, , idc] + bayer[idx + 1, idy, , idc] + bayer[idx, idy - 1, , idc] + bayer[idx, idy + 1, , idc]) / 4
  demosaiced[idx + 1, idy    , , idc] <- bayer[idx + 1, idy, , idc]
  demosaiced[idx    , idy + 1, , idc] <- bayer[idx, idy + 1, , idc]
  demosaiced[idx + 1, idy + 1, , idc] <- (bayer[idx, idy + 1, , idc] + bayer[idx + 2, idy + 1, , idc] + bayer[idx + 1, idy, , idc] + bayer[idx + 1, idy, , idc]) / 4
  idc <- 3 #B
  demosaiced[idx    , idy    , , idc] <- (bayer[idx - 1, idy - 1, , idc] + bayer[idx + 1, idy - 1, , idc] + bayer[idx - 1, idy + 1, , idc] + bayer[idx + 1, idy + 1, , idc]) / 4
  demosaiced[idx + 1, idy    , , idc] <- (bayer[idx + 1, idy - 1, , idc] + bayer[idx + 1, idy + 1, , idc]) / 2
  demosaiced[idx    , idy + 1, , idc] <- (bayer[idx - 1, idy + 1, , idc] + bayer[idx + 1, idy + 1, , idc]) / 2
  demosaiced[idx + 1, idy + 1, , idc] <- bayer[idx + 1, idy + 1, , idc]

  demosaiced
}

#重心一致9:3:3:1加重加算による縮小デモザイク
demosaic_w9331_downsampling <- function(bayer) {
  bayer
}
  
#重心一致9:3:3:1加重加算によるデモザイク
demosaic_w9331 <- function(bayer) {
  
  bayer
  #RGB隣接画素補間
  #G隣接画素補間/
  #Nearest Neigbor
  # I <- img_psf_blurred_rggb * 0.0
  # B <- img_psf_blurred_rggb
  # #R
  # idx <- seq(1, w, 2); idy <- seq(1, h, 2); idc <- 1
  # I[idx    , idy    , , idc] <- B[idx, idy, , idc] #@R
  # I[idx + 1, idy    , , idc] <- B[idx, idy, , idc] #@Gr
  # I[idx    , idy + 1, , idc] <- B[idx, idy, , idc] #@Gb
  # I[idx + 1, idy + 1, , idc] <- B[idx, idy, , idc] #@B
  #Gr
  # idx <- seq(2, w, 2); idy <- seq(1, h, 2); idc <- 2
  # I[idx    , idy    , , idc] <- B[idx, idy, , idc] #@Gr
  # I[idx - 1, idy    , , idc] <- B[idx, idy, , idc] #@R
  # #Gb
  # idx <- seq(1, w, 2); idy <- seq(2, h, 2); idc <- 2
  # I[idx    , idy    , , idc] <- B[idx, idy, , idc] #Gb
  # I[idx + 1, idy    , , idc] <- B[idx, idy, , idc] #B
  # #B
  # idx <- seq(2, w, 2); idy <- seq(2, h, 2); idc <- 3
  # I[idx    , idy    , , idc] <- B[idx, idy, , idc] #@B
  # I[idx - 1, idy    , , idc] <- B[idx, idy, , idc] #@Gb
  # I[idx    , idy - 1, , idc] <- B[idx, idy, , idc] #@Gr
  # I[idx - 1, idy - 1, , idc] <- B[idx, idy, , idc] #@R
  
  dim_img <- dim(bayer)
  w0 <- dim_img[1]; w <- floor(w0 / 2) * 2
  h0 <- dim_img[2]; h <- floor(h0 / 2) * 2
  demosaiced <- bayer * 0.0
  
  idx <- seq(3, w - 3, 2)
  idy <- seq(3, h - 3, 2)
  idc <- 1 #R
  demosaiced[idx    , idy    , , idc] <- (9*bayer[idx, idy, , idc] + 3*bayer[idx+2, idy, , idc] + 3*bayer[idx, idy+2, , idc] + 1*bayer[idx+2, idy+2, , idc]) / 16
  demosaiced[idx + 1, idy    , , idc] <- (3*bayer[idx, idy, , idc] + 9*bayer[idx+2, idy, , idc] + 1*bayer[idx, idy+2, , idc] + 3*bayer[idx+2, idy+2, , idc]) / 16
  demosaiced[idx    , idy + 1, , idc] <- (3*bayer[idx, idy, , idc] + 1*bayer[idx+2, idy, , idc] + 9*bayer[idx, idy+2, , idc] + 3*bayer[idx+2, idy+2, , idc]) / 16
  demosaiced[idx + 1, idy + 1, , idc] <- (1*bayer[idx, idy, , idc] + 3*bayer[idx+2, idy, , idc] + 3*bayer[idx, idy+2, , idc] + 9*bayer[idx+2, idy+2, , idc]) / 16
  idc <- 2 #G
  demosaiced[idx    , idy    , , idc] <- (9*bayer[idx+1, idy, , idc] + 3*bayer[idx-1, idy, , idc] + 3*bayer[idx+1, idy+2, , idc] + 1*bayer[idx-1, idy+2, , idc] +
                                            9*bayer[idx, idy+1, , idc] + 3*bayer[idx, idy-1, , idc] + 3*bayer[idx+2, idy+1, , idc] + 1*bayer[idx+2, idy-1, , idc]) / 32
  demosaiced[idx + 1, idy    , , idc] <- (9*bayer[idx+1, idy, , idc] + 3*bayer[idx+3, idy, , idc] + 3*bayer[idx+1, idy+2, , idc] + 1*bayer[idx+3, idy+2, , idc] +
                                            9*bayer[idx+2, idy+1, , idc] + 3*bayer[idx+2, idy-1, , idc] + 3*bayer[idx, idy+1, , idc] + 1*bayer[idx, idy-1, , idc]) / 32
  demosaiced[idx    , idy + 1, , idc] <- (9*bayer[idx+1, idy+2, , idc] + 3*bayer[idx+1, idy, , idc] + 3*bayer[idx-1, idy+2, , idc] + 1*bayer[idx-1, idy, , idc] +
                                            9*bayer[idx, idy+1, , idc] + 3*bayer[idx, idy+3, , idc] + 3*bayer[idx+2, idy+1, , idc] + 1*bayer[idx+2, idy+3, , idc]) / 32
  demosaiced[idx + 1, idy + 1, , idc] <- (9*bayer[idx+1, idy+2, , idc] + 3*bayer[idx+1, idy, , idc] + 3*bayer[idx+3, idy+2, , idc] + 1*bayer[idx+3, idy, , idc] +
                                            9*bayer[idx+2, idy+1, , idc] + 3*bayer[idx, idy+1, , idc] + 3*bayer[idx+2, idy+3, , idc] + 1*bayer[idx, idy+3, , idc]) / 32
  idc <- 3 #B
  demosaiced[idx    , idy    , , idc] <- (1*bayer[idx-1, idy-1, , idc] + 3*bayer[idx+1, idy-1, , idc] + 3*bayer[idx-1, idy+1, , idc] + 9*bayer[idx+1, idy+1, , idc]) / 16
  demosaiced[idx + 1, idy    , , idc] <- (1*bayer[idx+3, idy-1, , idc] + 3*bayer[idx+1, idy-1, , idc] + 3*bayer[idx+3, idy+1, , idc] + 9*bayer[idx+1, idy+1, , idc]) / 16
  demosaiced[idx    , idy + 1, , idc] <- (1*bayer[idx-1, idy+3, , idc] + 3*bayer[idx+1, idy+3, , idc] + 3*bayer[idx-1, idy+1, , idc] + 9*bayer[idx+1, idy+1, , idc]) / 16
  demosaiced[idx + 1, idy + 1, , idc] <- (1*bayer[idx+3, idy+3, , idc] + 3*bayer[idx+1, idy+3, , idc] + 3*bayer[idx+3, idy+1, , idc] + 9*bayer[idx+1, idy+1, , idc]) / 16
  
  demosaiced
}

demosaic <- function(img, camera, interpolation) {
  if(camera == "RGGB") {
    img_bayered <- bayerize(img)
    if(interpolation == "隣接画素縮小補間") {
      img_demosaiced <- demosaic_nearest_downsampling(img_bayered)
    }
    else if(interpolation == "隣接画素補間") {
      img_demosaiced <- demosaic_nearest(img_bayered)
    }
    # else if(interpolation == "重心一致縮小補間") {
    #   img_demosaiced <- demosaic_w9331_downsampling(img_bayered)
    # }
    else if(interpolation == "重心一致補間") {
      img_demosaiced <- demosaic_w9331(img_bayered)
    }
    else {
      img_demosaiced <- img_bayered
    }
    img_processed <- img_demosaiced
  } 
  else {
    img_demosaiced <- img
  }
  img_demosaiced
}

add_margin <- function(img, margin) {
  w0 <- dim(img)[1]
  h0 <- dim(img)[2]
  w <- w0 + 2 * margin
  h <- h0 + 2 * margin
  img_margin_added <- as.cimg(rep(0, w * h * 1 * 3), dim = c(w, h, 1, 3))
  idx <- 1:w0 + margin
  idy <- 1:h0 + margin
  img_margin_added[idx, idy, , ] <- img
  img_margin_added
}


