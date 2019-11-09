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
                         sliderInput("size", "Size", 32, 128, 32, animate = TRUE),
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
           plotOutput("image")
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
    
    image0[ , , 1] <- image0[ , , 1] / r_gain
    image0[ , , 3] <- image0[ , , 3] / b_gain
    #image0
    gain <- 1
    bayer <- image0[, , 2] / gain
    r_index <- seq(1, size, 2)
    b_index <- seq(1 + 1, size, 2)
    bayer[r_index, r_index] <- image0[r_index, r_index, 1] / gain / r_gain
    bayer[b_index, b_index] <- image0[b_index, b_index, 3] / gain / b_gain
    bayer <- bayer + sqrt(bayer) * rnorm(size * size, 0, 1)
    bayer <- bayer * gain
    bayer
    
  })
  
  output$image <- renderPlot({
    image0 <- create_image0()
    image <- image0 / max(image0)
    image <- pmax(image, 0.0)
    #image <- image ^ (1 / 2.2)
    
    g <- ggplot() + annotation_raster(image, -Inf, Inf, -Inf, Inf, interpolate = TRUE)
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
