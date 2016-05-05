set style line 1 lc rgb '#0060ad' lt 1 lw 2 pt 0 ps 1.5
set xlabel 'y1'
set ylabel 'y2'
set term svg
set output 'plot.svg'
plot 'dots' with linespoints ls 1
