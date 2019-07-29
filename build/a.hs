{-# LANGUAGE Strict #-}
{-# OPTIONS_GHC -O2 #-}
main::IO()
main = do
    readed <- map read . words <$> getLine
    putStr $ show (readed!!0 + readed!!1)
