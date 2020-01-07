source("./functions.R", encoding = "UTF8")
source("./settings.R", encoding = "UTF8")

#0:元画像のスケール
#1:シミュレーション分解能のスケール
#（無し）:結果画像データのスケール

#psf_seeing シーイング+シンチレーション
#psf_alignment 追尾/位置合わせ誤差
#psf_lens
#psf_airydisc
#psf_aperture
#psf_total

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
                    column(width = 3, numericInput("pix_um", "画素幅(um)", value = 1.35)),
                    column(width = 3, selectInput("sim_resolution", "Sim.倍率", choices = c(1, 2, 4, 8), selected = 1)),
                    column(width = 3, selectInput("sim_width", "画素幅", choices = c(8, 32, 64), selected = 32)),
                    column(width = 3, numericInput("sim_margin", "マージン(\")", value = 10))
                )
            )),
            column(width = 3, 
                   #            wellPanel( numericInput("num_exposure", "撮影枚数", value = 10)),
                   #numericInput("gamma", "Display Gamma", value = 2.2))
                   checkboxInput("gamma", label = "Gamma", value = TRUE),
                   radioButtons("camera", label = "Camera", c("3-Band","RGGB"), inline = TRUE, selected = default_camera),
                   selectInput("interpolation", "補間", choices = interpolations, selected = default_interpolation),
                   selectInput("upsampling", "アップサンプリング", choices = c(3, 5, 6), selected = 6)
                   #,
                   # fluidRow()
                   # slideInput("edge_enhance", "輪郭強調", min = 0, max = 2, value = 0))
            )
        ),
        # Show a plot of the generated distribution
        tabsetPanel(type = "tabs",
                    tabPanel("PSF", wellPanel(plotOutput("psfPlot", width = 1000, height = 500))),
                    tabPanel("Image", wellPanel(plotOutput("imagePlot", width = 1000, height = 500))),
                    selected = "Image"
        )
    )
)

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
        
        psf_seeing <- create_psf_gaussian(N_pix1, sigma_seeing_pix1)
        psf_optics <- create_psf_gaussian(N_pix1, sigma_optics_pix1)
        psf_airydisc <- create_psf_airydisc(N_pix1, f_ratio, pix1_um, lambda_um)
        psf_aperture <- create_psf_aperture(N_pix1, sim_resolution) #for display

        psf_sim <- convolve(psf_seeing, psf_optics)
        psf_sim <- convolve(psf_sim, psf_airydisc)
        psf_sim <- boxblur(psf_sim, sim_resolution)
        psf_sim[psf_sim < 0.0] <- 0.0
        psf_image <- imresize(psf_sim, scale = 1.0 / sim_resolution, interpolation = -1)

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
        sim_margin_arcsec <- input$sim_margin
        
        N_pix1 <- N_pix * sim_resolution
        pix1_um <- pix_um / sim_resolution
        
        pix_arcsec <- 180 / pi * 2 * atan(pix_um / 1000 / 2 / focal_length_mm) * 60 * 60
        pix1_arcsec <- pix_arcsec / sim_resolution
        
        sigma_seeing_arcsec <- input$sigma_seeing_arcsec
        sigma_seeing_pix1 <- sigma_seeing_arcsec / pix1_arcsec
        sigma_optics_pix1 <- sigma_optics_um / pix1_um
        sim_margin_pix1 <- ceiling(sim_margin_arcsec / pix1_arcsec)
        
        if(input$gamma == TRUE) {
            gamma <- 1 / 2.2
        }
        else { gamma <- 1.0 }
        
        psf_seeing <- create_psf_gaussian(N_pix1, sigma_seeing_pix1)
        psf_optics <- create_psf_gaussian(N_pix1, sigma_optics_pix1)
        psf_airydisc <- create_psf_airydisc(N_pix1, f_ratio, pix1_um, lambda_um)
        #psf_aperture <- create_psf_aperture(N_pix1, sim_resolution) #for display
        
        #Original image in size of simulation
        img1 <- imresize(img0, pix0_arcsec / pix1_arcsec, interpolation = 3)
        w0 <- dim(img1)[1]
        h0 <- dim(img1)[2]
        w <- w0 + 2 * sim_margin_pix1
        h <- h0 + 2 * sim_margin_pix1
        img1l <- as.cimg(rep(0, w * h * 1 * 3), dim = c(w, h, 1, 3))
        idx <- 1:w0 + sim_margin_pix1
        idy <- 1:h0 + sim_margin_pix1
        img1l[idx, idy, , ] <- img1
        img1 <- img1l
        
        psf_sim <- convolve(psf_seeing, psf_optics)
        psf_sim <- convolve(psf_sim, psf_airydisc)
        #psf_sim <- boxblur(psf_sim, sim_resolution) # changed to apply img1_blurred
        psf_sim[psf_sim < 0.0] <- 0.0
        #Blurred image on the sensor in size of simulation
        img1_blurred <- convolve(img1, psf_sim)
       
        #img <- imresize(img1_blurred_boxblurred, scale = 1.0 / sim_resolution, interpolation = 1)
        #Calculation for aperture MTF
        img1_captured <- boxblur(img1_blurred, sim_resolution)
        #Blurred image captured by 3-band sensor
        img1_captured <- imresize(img1_captured, scale = 1.0 / sim_resolution, interpolation = -1)
        img1_captured[img1_captured < 0.0] <- 0.0
        
        dim_img <- dim(img1_captured)

        img_processed <- demosaic(img1_captured, input$camera, input$interpolation)
        img_upsampled <- resize(img_processed, 
                                size_x = dim_img[1] * sim_resolution, 
                                size_y = dim_img[2] * sim_resolution, interpolation_type = as.integer(input$upsampling))

        pw <- 256
        par(mar = c(0, 0, 1.5, 0))
        plot( 0, 0, xlim = c(1, pw * 2), ylim = c(pw * 2, 1), 
              type="n", asp = dim_img[2] / dim_img[1], axes = FALSE, xlab = "", ylab = "" )
        title(glue("Focal Length={focal_length_mm}(mm) / Pixel Size={pix_um}(um) / Sim. Resolution={sim_resolution}"), col.main = "white")
        m <- 0; n <- 0
        print(dev.list())      
        
        rasterImage(img1^gamma, 0 * pw + 1, pw, (0 + 1) * pw, 1, interpolate = FALSE)
        text(1, 1, glue("Original"), col = "white", adj = c(0, 1))

        rasterImage(img1_blurred^gamma, 1 * pw + 1, (0 + 1) * pw + 1, (1 + 1) * pw, (0) * pw + 1, interpolate = FALSE)
        text(1 * pw + 1, 0*pw + 1, glue("Blurred Original"), col = "yellow", adj = c(0, 1))
        
        rasterImage(img_processed^gamma, 0 * pw + 1, (1 + 1) * pw + 1, (0 + 1) * pw, (1) * pw + 1, interpolate = FALSE)
        text(0*pw + 1, 1*pw + 1, glue("Captured by {input$camera} camera"), col = "cyan", adj = c(0, 1))
        
        rasterImage(img_upsampled^gamma, 1 * pw + 1, (1 + 1) * pw + 1, (1 + 1) * pw, (1) * pw + 1, interpolate = FALSE)
        text(1*pw + 1, 1*pw + 1, glue("Captured by {input$camera} camera : Upsampled."), col = "magenta", adj = c(0, 1))
    }, bg = "gray20")
}

# Run the application 
shinyApp(ui = ui, server = server)