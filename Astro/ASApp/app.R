#App for Astrophotography Simulator

library(shiny)
library(shinythemes)
library(tidyverse)
library(glue)
library(imager)

source("variables.R", encoding = "UTF-8")
source("functions.R", encoding = "UTF-8")

# tab
# reactive
# speed

r_gain <- 2.0
b_gain <- 1.5
bayer_offset <- 1008


# Define UI for application that draws a histogram
ui <- fluidPage(
  theme = shinytheme("slate"),
  #theme = "bootstrap.min.css",
  # Application title
  titlePanel("Astrophotograpy Simulator"),
  fluidRow(
    column(4, 
           #           navlistPanel("A",
           tabsetPanel(type = "tabs",
                       tabPanel("対象", wellPanel(
                         selectInput("object", "Object", object_names, selectize = TRUE),
                         sliderInput("size", "Size", 32, 128, 32, step = 2, animate = TRUE),
                         #                         numericInput("size", "Size", 256, width = 80),
                         sliderInput("background_level", "Background level (e/s/pix2)", 0, 255, 128),
                         sliderInput("object_level", "Object level (e/s/star)", 0, 255, 128),
                         sliderInput("psigma", "Object sigma", 0, 10, 1),
                       )),
                       tabPanel("カメラ", wellPanel(
                         fluidRow(
                           column(6, selectInput("camera", "Camera", camera_names, selectize = TRUE)),
                           column(6, selectInput("iso", "ISO", isos, selectize = TRUE))
                         ),
                         numericInput("focal_length_mm", "FocalLength(mm)", 800, width = 80),
                         br(),
                         tableOutput("camera_values")
                       )),
                       tabPanel("撮影", wellPanel(
                       )),
                       tabPanel("処理", wellPanel(
                       ))
           )
    ),
    column(8,
           plotOutput("image"),
           fluidRow(
             column(3, radioButtons("display", "", c("Original", "Bayer", "Processed"), 
                                    selected = "Processed")),
             column(6, sliderInput("display_range", "Range", 0, 4095, c(0, 255))),
             column(3,
                    checkboxInput("display_gamma", "Gamma", TRUE), 
                    checkboxInput("display_interpolate", "Interpolate", FALSE)
             )
           )
           #    plotOutput("plot")
    )
  )
)

# Define server logic required to draw a histogram
server <- function(input, output) {
  cameraValues <- reactive({
    p <- filter(camera_params, Camera == input$camera)
    iso <- as.integer(input$iso)
    gain_ADU_e <- p$gain0_ADU_e * iso / 100
    data.frame(
      Param = c("Camera",
                "ISO",
                "Pitch(um/pixel)",
                "Gain(ADU/e)",
                "Sigma(e)"),
      Value = as.character(c(p$Camera,
                             iso,
                             p$um_pix,
                             gain_ADU_e,
                             p$nsigma_rn_e)),
      stringsAsFactors = FALSE)
  })
  
  # Show the values in an HTML table ----
  output$camera_values <- renderTable({
    cameraValues()
  })
  
  create_image0 <- reactive({
    size <- input$size
    xc <- size / 2
    yc <- size / 2
    object_level_e_s_pix2 <- input$object_level
    background_level_e_s_pix2 <- input$background_level
    psigma <- input$psigma
    image0 <- create_star_image(size, size, xc, yc, 
                                object_level_e_s_pix2, psigma)
    image0 <- image0 + background_level_e_s_pix2
    
    image0
  })
  create_bayer <- reactive({
    image0 <- create_image0()
    size <- dim(image0)[1]
    
    gain <- 1
    bayer <- image0[, , 2] / gain
    r_index <- seq(1, size, 2)
    b_index <- seq(1 + 1, size, 2)
    bayer[r_index, r_index] <- image0[r_index, r_index, 1] / gain / r_gain
    bayer[b_index, b_index] <- image0[b_index, b_index, 3] / gain / b_gain
    bayer <- bayer + sqrt(bayer) * rnorm(size * size, 0, 1)
    bayer <- bayer * gain
    bayer <- bayer + bayer_offset
    bayer
  })
  create_processed <- reactive({
    bayer <- create_bayer()
    bayer <- bayer - bayer_offset
    size <- dim(bayer)[1]
    
    #start 0,0 on image
    index_e <- seq(0, size - 1, 2) + 1
    index_o <- seq(1, size - 1, 2) + 1
    index_em <- seq(2    , size - 2 - 1, 2) + 1
    index_om <- seq(2 + 1, size - 2 - 1, 2) + 1
    index_em1 <- seq(0    , size - 2 - 1, 2) + 1
    index_om1 <- seq(0 + 1, size - 2 - 1, 2) + 1
    index_em2 <- seq(2    , size - 1, 2) + 1
    index_om2 <- seq(2 + 1, size - 1, 2) + 1
    
    bayer[index_e, index_e] <- bayer[index_e, index_e] * r_gain
    bayer[index_o, index_o] <- bayer[index_o, index_o] * b_gain
    image <- array(0.0, dim = c(size, size, 3))
    image[index_e, index_e, 1] <- bayer[index_e, index_e]
    image[index_e, index_o, 2] <- bayer[index_e, index_o]
    image[index_o, index_e, 2] <- bayer[index_o, index_e]
    image[index_o, index_o, 3] <- bayer[index_o, index_o]
    print(image)
    
    image[index_em2, index_em2, 2] <- (
      image[index_em2    , index_em2 - 1, 2] +
        image[index_em2    , index_em2 + 1, 2] +
        image[index_em2 - 1, index_em2    , 2] +
        image[index_em2 + 1, index_em2    , 2]) / 4
    image[index_om1, index_om1, 2] <- (
      image[index_om1    , index_om1 - 1, 2] +
        image[index_om1    , index_om1 + 1, 2] +
        image[index_om1 - 1, index_om1    , 2] +
        image[index_om1 + 1, index_om1    , 2]) / 4
    #interpolate R
    # print(image[, , 1])
    # print(image[index_em, index_em, 1])
    image[index_em2, index_om1, 1] <- (
      image[index_em2    , index_om1 - 1, 1] +
        image[index_em2    , index_om1 + 1, 1]) / 2
    image[index_om1, index_om1, 1] <- (
      bayer[index_om1 - 1, index_om1 - 1] +
        bayer[index_om1 - 1, index_om1 + 1] +
        bayer[index_om1 + 1, index_om1 - 1] +
        bayer[index_om1 + 1, index_om1 + 1]) / 4
    image[index_om1, index_em2, 1] <- (
      image[index_om1 - 1  , index_em2, 1] +
        image[index_om1 + 1, index_em2, 1]) / 2
    
    image[index_em2, index_om1, 3] <- (
      bayer[index_em2 - 1, index_om1] +
        bayer[index_em2 + 1, index_om1]) / 2
    image[index_em2, index_em2, 3] <- (
      bayer[index_em2 - 1, index_em2 - 1] +
        bayer[index_em2 - 1, index_em2 + 1] +
        bayer[index_em2 + 1, index_em2 - 1] +
        bayer[index_em2 + 1, index_em2 + 1]) / 4
    image[index_om1, index_em2, 3] <- (
      bayer[index_om1  , index_em2 - 1] +
        bayer[index_om1, index_em2 + 1]) / 2
    # image[,,1] <- 0
    # image[,,2] <- 0
#    image[,,3] <- 0
    
    image
  })
  
  output$image <- renderPlot({
    if(input$display == "Original") {
      image <- create_image0()
    }
    else if(input$display == "Bayer") {
      image <- create_bayer()
      image <- image - bayer_offset
    }
    else if(input$display == "Processed") {
      image <- create_processed()
    }
    
    size <- dim(image)
    #image <- image0 / max(image0)
    display_range <- input$display_range
    
    image <- (image - display_range[1]) / (display_range[2] - display_range[1])
    image <- pmax(image, 0.0)
    image <- pmin(image, 1.0)
    if(input$display_gamma) {
      image <- image ^ (1 / 2.2)
    }
    interpolate <- input$display_interpolate
    g <- ggplot() + annotation_raster(image, -Inf, Inf, -Inf, Inf, interpolate = interpolate)
    #g <- ggplot() + annotation_raster(image, 1, size, -1, -size, interpolate = TRUE)
    g <- g + coord_equal(xlim = c(1, size), ylim = c(1, size))
    #g <- g + coord_equal(xlim = c(1, size), ylim = c(-size, size))
    g <- g + theme(plot.background = element_rect(fill = "transparent", color = NA),
                   panel.background = element_rect(fill = "transparent", color = NA))
    g
  }, bg = "black")
  
  output$plot <- renderPlot({
    # generate bins based on input$bins from ui.R
    x <- 1:256
    max_object_level_e_s <- input$max_object_level
    background_level_e_s <- input$background_level
    y <- background_level_e_s + max_object_level_e_s 
    g <- ggplot() + geom_point(aes(x, y))
    g <- g + geom_line(aes(x, background_level_e_s), color = "red")
    g
  })
}

# Run the application 
shinyApp(ui = ui, server = server)
