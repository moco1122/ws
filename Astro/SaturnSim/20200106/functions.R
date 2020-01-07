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

