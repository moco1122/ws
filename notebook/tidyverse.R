#tidyverseを利用してよく使うパターンを関数化

library(tidyverse) # ggplot2, dplyr, tidyr, readr, purrr, tibble, stringr, forcats
glue <- glue::glue # tidyrのcollapse()と衝突すると警告が出たので
library(patchwork)

#Sys.setlocale('LC_ALL','C')

#col_types推測のメッセージが出ないようにするオプション
read_csv0 <- function(file) { 
  read_csv(file, col_names =TRUE, comment = "#", col_types = cols())
}

#purrr::map*を想定したヘルパー関数
#broom::tidyの出力を係数部分だけにして整形
tidy_coef <- function(model, col_names = NULL) { 
  coef_dat <- broom::tidy(model) %>%  select(1:2)
  
  if(!is.null(col_names)) {
    coef_dat  <- coef_dat %>% mutate(term = col_names)
  }
  print(coef_dat)
  coef_dat %>% spread(term, estimate) 
}
