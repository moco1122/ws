object_names <- c("Flat", "Star", "Stars", "M42", "M31", "Moon", "GradationFlat", "GradationText")
#None : flat/sky 
#   background_level_e_s
#Star : Single star
#   background_level_e_s, max_object_level_e_s
#Stars : Multiple stars
#   background_level_e_s, max_object_level_e_s, star_pattern(random/named)
#M42 : Great Orion Nebula 
#   background_level_e_s, max_object_level_e_s
#M31 : Andromeda Galaxy 
#   background_level_e_s, max_object_level_e_s
#Moon : Moon
#   background_level_e_s, max_object_level_e_s, moon_age
#Gradation : Gradation flat chart
#   background_level_e_s, max_object_level_e_s
#GradationText : Gradation Text chart
#   background_level_e_s, max_object_level_e_s

camera_names <- c("D810A")
isos <- c(100, 200, 400, 800, 1600, 3200, 6400, 12800, 25600, 51200)

camera_params <- tribble(
  ~Camera, ~um_pix, ~gain0_ADU_e, ~nsigma_rn_e,
  "D810A",     4.3,           10,            5,
)