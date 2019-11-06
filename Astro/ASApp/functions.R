gaussian <- function(x, y, xc, yc, sigma) {
  exp(-((x - xc)^2 + (y - yc)^2) / (2 * sigma ^2)) / sqrt(2 * pi) / sigma
}

create_star_image <- function(width, height, xc, yc, brightness, sigma) {
  xy <- expand.grid(y = 1:height, x = 1:width) # in this order to
  
  if(length(xc) == 1) { xc <- rep(xc, 3); }
  else if(length(xc) != 3) { stop("length(xc) != 3") }
  if(length(yc) == 1) { yc <- rep(yc, 3); }
  else if(length(yc) != 3) { stop("length(yc) != 3") }
  if(length(brightness) == 1) { brightness <- rep(brightness, 3); }
  else if(length(brightness) != 3) { stop("length(brightness) != 3") }
  if(length(sigma) == 1) { sigma <- rep(sigma, 3); }
  else if(length(sigma) != 3) { stop("length(sigma) != 3") }
  
  rgb <- xy %>% 
    mutate(r = brightness[1] * gaussian(x, y, xc[1], yc[1], sigma[1]),
           g = brightness[2] * gaussian(x, y, xc[2], yc[2], sigma[2]),
           b = brightness[3] * gaussian(x, y, xc[3], yc[3], sigma[3]))
  image <- array(c(rgb$r, rgb$g, rgb$b), dim = c(height, width, 3))
  image
}