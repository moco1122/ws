library(Rcpp)
library(imager)

#sourceCpp(file = "/ws/Astrophotography/src/dcrawRcpp.cpp", rebuild = TRUE)

# system.time(b1 <- read_NEF_as_bayer("/Volumes/Photo/astrophoto/2011/20110320D90/NEF/DSC_3120.NEF"))
# c1 <- as.cimg(b1)
# tc1 <- resize(c1,round(width(c1)/10),round(height(c1)/10))


system.time(s2 <- read_NEF_as_srgb("/Volumes/Photo/astrophoto/2011/20110320D90/NEF/DSC_3120.NEF"))
s2[,,1] = s2[,,1] * 1.5
s2[,,3] = s2[,,3] * 1.5
c2 <- as.cimg(s2)
tc2 <- resize(c2,round(width(c2)/10),round(height(c2)/10))
plot(tc2)

system.time(s1 <- read_NEF_as_srgb("/Volumes/Photo/astrophoto/2011/20110320D90/NEF/DSC_3120.NEF", FALSE))
s1[,,1] = s1[,,1] * 1.5; s1[,,3] = s1[,,3] * 1.5
system.time(s2 <- read_NEF_as_srgb("/Volumes/Photo/astrophoto/2011/20110320D90/NEF/DSC_3120.NEF", FALSE))
s2[,,1] = s2[,,1] * 1.5; s2[,,3] = s2[,,3] * 1.5

x1 <- 701; y1 <- 951
d1 <- as.cimg(s1[y1:(y1+100),x1:(x1+100),])
d2 <- as.cimg(s2[y1:(y1+100),x1:(x1+100),])
layout(t(1:2))
plot(d1)
plot(d2)

#ガンマかけて保存してPs/Lrで比較
#Shinyビューア
#縦横確認


require(grDevices)
## set up the plot region:
op <- par(bg = "black")
plot(c(1, 100), c(1, 100), type = "n", xlab = "", ylab = "")
image <- as.raster(matrix(0:1, ncol = 5, nrow = 3))
ts2 <- s2[y1:(y1+100),x1:(x1+100),]
ts2 <- ts2 / max(ts2)
ts2 <- s2 / max(s2)
ts2 <- s2[601:1300, 601:1300, ] / max(s2)
ts2 <- ts2^(1/2.2)
whc <- dim(ts2); x1 <- 1; x2 <- whc[2]; y1 <- 1; y2 <- whc[1]
plot(c(x1, x2), c(y1, y2), type = "n", xlab = "", ylab = "", asp=1)
system.time(rasterImage(ts2, x1, y1, x2, y2, interpolate = FALSE))

#wb,gai