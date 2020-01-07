#
# This is a Shiny web application. You can run the application by clicking
# the 'Run App' button above.
#
# Find out more about building applications with Shiny here:
#
#    http://shiny.rstudio.com/
#
source("./functions.R")

#psf_seeing シーイング+シンチレーション
#psf_alignment 追尾/位置合わせ誤差
#psf_lens
#psf_airydisc
#psf_aperture
#psf_total
default_camera <- "RGGB"
# Define UI for application that draws a histogram
ui <- fluidPage(
    withMathJax(), 
    theme = "bootstrap.min.css",  #wwwフォルダーの中のcssファイルを指定
    
    # Application title
    titlePanel("Saturn Sim"),
    
    # Sidebar with a slider input for number of bins 
    sidebarLayout(
        fluidRow(
            column(width = 3, wellPanel(
                numericInput("sigma_seeing_arcsec", "シーイングσ(\")", value = 1.5),
                numericInput("rms_alignment_arcsec", "位置合わせ誤差RMS(\")", value = 0.5)
            )),
            column(width = 6, wellPanel(
                fluidRow(
                    column(width = 4, numericInput("focal_length_mm", "焦点距離(mm)", value = 539)),
                    column(width = 4, selectInput("f_ratio", "F値", choices = c(0.95, 1.4, 2.0, 2.8, 4.0, 5.6, 8.0, 11.0, 16.0), selected = 8)),
                    column(width = 4, numericInput("sigma_optics_um", "光学系σ(um)", value = 1.0))
                ),
                fluidRow(
                    column(width = 4, numericInput("pix_um", "画素幅(um)", value = 1.35)),
                    column(width = 4, selectInput("sim_resolution", "Sim.倍率", choices = c(1, 2, 4, 8), selected = 1)),
                    column(width = 4, selectInput("sim_width", "Sim.画素幅", choices = c(8, 32, 64), selected = 32))
                )
            )),
            column(width = 3, 
                   #            wellPanel( numericInput("num_exposure", "撮影枚数", value = 10)),
                   #numericInput("gamma", "Display Gamma", value = 2.2))
                   checkboxInput("gamma", label = "Gamma", value = TRUE),
                   radioButtons("camera", label = "Camera", c("3-Band","RGGB"), inline = TRUE, selected = default_camera))
        ),
        # Show a plot of the generated distribution
        tabsetPanel(type = "tabs",
                    tabPanel("PSF", wellPanel(plotOutput("psfPlot", width = 1000, height = 500))),
                    tabPanel("Image", wellPanel(plotOutput("imagePlot", width = 1000, height = 500)))
        )
    )
)

#0:元画像のスケール
#img0 <- load.image("D:/astrophoto/sorabiz/image/Saturn_HST_2004-03-22.jpg")
img0 <- load.image("/ws/Astro/ASApp/img//Saturn_HST_2004-03-22.jpg")
img0 <- (img0 / max(img0)) ^ (2.2)
pix0_arcsec <- 19.43 / 890
#1:シミュレーション分解能のスケール
#（無し）:結果画像データのスケール

# Define server logic required to draw a histogram
server <- function(input, output) {
    #arcsec_pix <- 0.51662
    
    output$psfPlot <- renderPlot({
        focal_length_mm <- input$focal_length_mm
        f_ratio <- as.numeric(input$f_ratio)
        sigma_optics_um <- as.numeric(input$sigma_optics_um)
        N_pix <- as.numeric(input$sim_width)
        sim_resolution <- as.numeric(input$sim_resolution)
        pix_um <- input$pix_um
        lambda_um <- 500 * 10^(-3)
        
        N_pix1 <- N_pix * sim_resolution
        pix1_um <- pix_um / sim_resolution
        
        pix_arcsec <- 180 / pi * 2 * atan(pix_um / 1000 / 2 / focal_length_mm) * 60 * 60
        pix1_arcsec <- pix_arcsec / sim_resolution
        
        sigma_seeing_arcsec <- input$sigma_seeing_arcsec
        sigma_seeing_pix1 <- sigma_seeing_arcsec / pix1_arcsec
        sigma_optics_pix1 <- sigma_optics_um / pix1_um
        gamma <- 1 / input$gamma
        
        psf_seeing <- create_psf_seeing(N_pix1, sigma_seeing_pix1)
        # psf_seeing <- as.cimg(function(x, y) {
        #     exp(-((x - N_pix1 / 2)^2 + (y - N_pix1 / 2)^2)/(2 * sigma_seeing_pix1^2))}, N_pix1, N_pix1)
        
        if(sigma_optics_pix1 <= 0.0) {
            psf_optics <- as.cimg(function(x, y) { x * 0.0 }, N_pix1, N_pix1)
            psf_optics[N_pix1 / 2, N_pix1 / 2, ,] <- 1
        }
        else {
            psf_optics <- as.cimg(function(x, y) {
                exp(-((x - N_pix1 / 2)^2 + (y - N_pix1 / 2)^2)/(2 * sigma_optics_pix1^2))}, N_pix1, N_pix1)
        }
        
        psf_airydisc <- as.cimg(function(x, y) {
            r_um <- sqrt((x - N_pix1 / 2)^2 + (y - N_pix1 / 2)^2) * pix1_um; 
            xx <- pi * r_um / lambda_um / f_ratio;
            (2 * besselJ( xx, 1) / xx)^2  }, N_pix1, N_pix1)
        psf_airydisc[N_pix1 / 2, N_pix1 / 2, ,] <- 1
        
        psf_aperture <- as.cimg(function(x, y) { x * 0.0 }, N_pix1, N_pix1)
        psf_aperture[(N_pix1 / 2 - sim_resolution / 2):(N_pix1 / 2 + sim_resolution / 2 - 1), 
                     (N_pix1 / 2 - sim_resolution / 2):(N_pix1 / 2 + sim_resolution / 2 - 1), ,] <- 1
        
        psf_sim <- convolve(psf_seeing, psf_optics)
        psf_sim <- convolve(psf_sim, psf_airydisc)
        psf_sim <- boxblur(psf_sim, sim_resolution)
        psf_sim[psf_sim < 0.0] <- 0.0
        psf_image <- imresize(psf_sim, scale = 1.0 / sim_resolution, interpolation = -1)
        
        #psf_image0 <- convolve(psf_focalplane)
        
        pw <- 256
        par(mar = c(0, 0, 1.5, 0))
        plot( 0, 0, xlim = c(1, pw * 4), ylim = c(pw * 2, 1), 
              type="n", asp=1, axes = FALSE, xlab = "", ylab = "" )
        title(glue("Focal Length={focal_length_mm}(mm) / Pixel Size={pix_um}(um) / Sim. Resolution={sim_resolution}"), col.main = "white")
        m <- 0; n <- 0
        
        rasterImage(psf_seeing^gamma, m * pw + 1, pw, (m + 1) * pw, 1, interpolate = FALSE)
        text(m * pw + 1, 1, glue("Seeing={sigma_seeing_arcsec}\" "), col = "red", adj = c(0, 1)); m <- m + 1
        
        rasterImage(psf_optics^gamma, m * pw + 1, pw, (m + 1) * pw, 1, interpolate = FALSE)
        text(m * pw + 1, 1, glue("Optics sigma={sigma_optics_um}(um)"), col = "yellow", adj = c(0, 1)); m <- m + 1
        
        rasterImage(psf_airydisc^gamma, m * pw + 1, pw, (m + 1) * pw, 1, interpolate = FALSE)
        text(m * pw + 1, 1, glue("Airy disc F/{f_ratio} {lambda_um * 1000}(nm)"), col = "green", adj = c(0, 1)); m <- m + 1
        
        rasterImage(psf_aperture^gamma, m * pw + 1, pw, (m + 1) * pw, 1, interpolate = FALSE)
        text(m * pw + 1, 1, glue("Pixel Aperture"), col = "blue", adj = c(0, 1)); m <- m + 1
        
        m <- 0; n <- 1;
        rasterImage(psf_sim^gamma, m * pw + 1, (n + 1) * pw + 1, (m + 1) * pw, n * pw + 1, interpolate = FALSE)
        text(m * pw + 1, n * pw + 1, glue("PSF(Sim.)"), col = "magenta", adj = c(0, 1)); m <- m + 1
        
        rasterImage(psf_image^gamma, m * pw + 1, (n + 1) * pw + 1, (m + 1) * pw, n * pw + 1, interpolate = FALSE)
        text(m * pw + 1, n * pw + 1, glue("PSF(Image)"), col = "cyan", adj = c(0, 1)); m <- m + 1
        
    }, bg = "gray20")
    
    output$imagePlot <- renderPlot({
        focal_length_mm <- input$focal_length_mm
        f_ratio <- as.numeric(input$f_ratio)
        sigma_optics_um <- as.numeric(input$sigma_optics_um)
        N_pix <- as.numeric(input$sim_width)
        sim_resolution <- as.numeric(input$sim_resolution)
        pix_um <- input$pix_um
        lambda_um <- 500 * 10^(-3)
        
        N_pix1 <- N_pix * sim_resolution
        pix1_um <- pix_um / sim_resolution
        
        pix_arcsec <- 180 / pi * 2 * atan(pix_um / 1000 / 2 / focal_length_mm) * 60 * 60
        pix1_arcsec <- pix_arcsec / sim_resolution
        
        sigma_seeing_arcsec <- input$sigma_seeing_arcsec
        sigma_seeing_pix1 <- sigma_seeing_arcsec / pix1_arcsec
        sigma_optics_pix1 <- sigma_optics_um / pix1_um
        if(input$gamma == TRUE) {
            gamma <- 1 / 2.2
        }
        else { gamma <- 1.0 }
        
        psf_seeing <- as.cimg(function(x, y) {
            exp(-((x - N_pix1 / 2)^2 + (y - N_pix1 / 2)^2)/(2 * sigma_seeing_pix1^2))}, N_pix1, N_pix1)
        
        if(sigma_optics_pix1 <= 0.0) {
            psf_optics <- as.cimg(function(x, y) { x * 0.0 }, N_pix1, N_pix1)
            psf_optics[N_pix1 / 2, N_pix1 / 2, ,] <- 1
        }
        else {
            psf_optics <- as.cimg(function(x, y) {
                exp(-((x - N_pix1 / 2)^2 + (y - N_pix1 / 2)^2)/(2 * sigma_optics_pix1^2))}, N_pix1, N_pix1)
        }
        
        psf_airydisc <- as.cimg(function(x, y) {
            r_um <- sqrt((x - N_pix1 / 2)^2 + (y - N_pix1 / 2)^2) * pix1_um; 
            xx <- pi * r_um / lambda_um / f_ratio;
            (2 * besselJ( xx, 1) / xx)^2  }, N_pix1, N_pix1)
        psf_airydisc[N_pix1 / 2, N_pix1 / 2, ,] <- 1
        
        psf_aperture <- as.cimg(function(x, y) { x * 0.0 }, N_pix1, N_pix1)
        psf_aperture[(N_pix1 / 2 - sim_resolution / 2):(N_pix1 / 2 + sim_resolution / 2 - 1), 
                     (N_pix1 / 2 - sim_resolution / 2):(N_pix1 / 2 + sim_resolution / 2 - 1), ,] <- 1
        
        img1 <- imresize(img0, pix0_arcsec / pix1_arcsec, interpolation = 3)
        print(img0)
        print(img1)
        psf_sim <- convolve(psf_seeing, psf_optics)
        psf_sim <- convolve(psf_sim, psf_airydisc)
        psf_sim <- boxblur(psf_sim, sim_resolution)
        psf_sim[psf_sim < 0.0] <- 0.0
        img1_psf_blurred <- convolve(img1, psf_sim)
        
        
        img <- imresize(img1, scale = 1.0 / sim_resolution, interpolation = 1)
        #Calculation for aperture MTF
        img_psf_blurred <- imresize(img1_psf_blurred, scale = 1.0 / sim_resolution, interpolation = -1)
        img_psf_blurred[img_psf_blurred < 0.0] <- 0.0
        
        #img_upsampled <- imresize(img_psf_blurred, scale = sim_resolution, interpolation = 6)
        
        pw <- 256
        par(mar = c(0, 0, 1.5, 0))
        plot( 0, 0, xlim = c(1, pw * 2), ylim = c(pw * 2, 1), 
              type="n", asp = dim(img)[2] / dim(img)[1], axes = FALSE, xlab = "", ylab = "" )
        title(glue("Focal Length={focal_length_mm}(mm) / Pixel Size={pix_um}(um) / Sim. Resolution={sim_resolution}"), col.main = "white")
        m <- 0; n <- 0
        
        rasterImage(img1^gamma, 0 * pw + 1, pw, (0 + 1) * pw, 1, interpolate = FALSE)
        text(1, 1, glue("Original"), col = "white", adj = c(0, 1))
        
        # rasterImage(img^gamma, 1 * pw + 1, pw, (1 + 1) * pw, 1, interpolate = FALSE)
        # text(pw + 1, 1, glue("Captured without blur"), col = "yellow", adj = c(0, 1))
        
        rasterImage(img1_psf_blurred^gamma, 1 * pw + 1, (0 + 1) * pw + 1, (1 + 1) * pw, (0) * pw + 1, interpolate = FALSE)
        text(1 * pw + 1, 0*pw + 1, glue("Blurred Original"), col = "yellow", adj = c(0, 1))
        
        if(input$camera == "RGGB") {
            w0 <- dim(img_psf_blurred)[1]; w <- ceiling(w0 / 2) * 2
            h0 <- dim(img_psf_blurred)[2]; h <- ceiling(h0 / 2) * 2
            B <- cimg(array(0.0, c(w, h, 1, 3)))
            print(w0)
            print(h0)
            print(B)
            
            #img_psf_blurred[seq(1, w, 2), seq(1, w, 2), , 1] <- 0.0
            idx0 <- seq(1, floor(w0 / 2) * 2, 2); idy0 <- seq(1, floor(h0 / 2) * 2, 2); 
            B[idx0    , idy0    , , 1] <- img_psf_blurred[idx0    , idy0    , , 1]
            B[idx0 + 1, idy0    , , 2] <- img_psf_blurred[idx0 + 1, idy0    , , 2]
            B[idx0    , idy0 + 1, , 2] <- img_psf_blurred[idx0    , idy0 + 1, , 2]
            B[idx0 + 1, idy0 + 1, , 3] <- img_psf_blurred[idx0 + 1, idy0 + 1, , 3]
            img_psf_blurred <- B
            
            #Nearest Neigbor x1/2Downsampling
            I <- imresize(B, 0.5, -1) * 0.0
            idx0 <- seq(1, w0, 2); idy0 <- seq(1, h0, 2); idx <- 1:(w / 2); idy <- 1:(h / 2); 
            #R
            idc <- 1; I[idx    , idy    , , idc] <- B[idx0, idy0, , idc] 
            #G
            idc <- 2; I[idx    , idy    , , idc] <- (B[idx0 + 1, idy0, , idc] + B[idx0    , idy0 + 1, , idc]) / 2
            #B
            idc <- 3; I[idx    , idy    , , idc] <- B[idx0 + 1, idy0 + 1, , idc]
            
            #重心一致 x1/2 DownSampling
            #重心一致 x1
            
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
            
            img_psf_blurred <- I
        }
        img_upsampled <- resize(img_psf_blurred, 
                                size_x = dim(img_psf_blurred)[1] * sim_resolution, 
                                size_y = dim(img_psf_blurred)[2] * sim_resolution, interpolation_type = 3L)
        
        print(img_psf_blurred)
        print(img_upsampled)
        print(img_upsampled[1:5,1:5,,1])
        rasterImage(img_psf_blurred^gamma, 0 * pw + 1, (1 + 1) * pw + 1, (0 + 1) * pw, (1) * pw + 1, interpolate = FALSE)
        text(0*pw + 1, 1*pw + 1, glue("Captured by 3-band camera"), col = "cyan", adj = c(0, 1))
        
        rasterImage(img_upsampled^gamma, 1 * pw + 1, (1 + 1) * pw + 1, (1 + 1) * pw, (1) * pw + 1, interpolate = FALSE)
        text(1*pw + 1, 1*pw + 1, glue("Captured by 3-band camera : Upsampled."), col = "magenta", adj = c(0, 1))
    }, bg = "gray20")
}

# Run the application 
shinyApp(ui = ui, server = server)