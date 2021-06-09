#!/bin/bash
python calc_query_performance.py ostrich/spo-query.txt cobra_opt/spo-query.txt 8
python make_mean_line_plot.py ostrich/spo-query.txt cobra_opt/spo-query.txt 8

python calc_query_performance.py ostrich/low-so-query.txt cobra_opt/low-so-query.txt 8
python make_mean_line_plot.py ostrich/low-so-query.txt cobra_opt/low-so-query.txt 8

python calc_query_performance.py ostrich/low-sp-query.txt cobra_opt/low-sp-query.txt 8
python make_mean_line_plot.py ostrich/low-sp-query.txt cobra_opt/low-sp-query.txt 8

python calc_query_performance.py ostrich/high-sp-query.txt cobra_opt/high-sp-query.txt 8
python make_mean_line_plot.py ostrich/high-sp-query.txt cobra_opt/high-sp-query.txt 8

python calc_query_performance.py ostrich/low-po-query.txt cobra_opt/low-po-query.txt 8
python make_mean_line_plot.py ostrich/low-po-query.txt cobra_opt/low-po-query.txt 8

python calc_query_performance.py ostrich/high-po-query.txt cobra_opt/high-po-query.txt 8
python make_mean_line_plot.py ostrich/high-po-query.txt cobra_opt/high-po-query.txt 8

python calc_query_performance.py ostrich/low-s-query.txt cobra_opt/low-s-query.txt 8
python make_mean_line_plot.py ostrich/low-s-query.txt cobra_opt/low-s-query.txt 8

python calc_query_performance.py ostrich/high-s-query.txt cobra_opt/high-s-query.txt 8
python make_mean_line_plot.py ostrich/high-s-query.txt cobra_opt/high-s-query.txt 8

python calc_query_performance.py ostrich/low-o-query.txt cobra_opt/low-o-query.txt 8
python make_mean_line_plot.py ostrich/low-o-query.txt cobra_opt/low-o-query.txt 8

python calc_query_performance.py ostrich/high-o-query.txt cobra_opt/high-o-query.txt 8
python make_mean_line_plot.py ostrich/high-o-query.txt cobra_opt/high-o-query.txt 8

python calc_query_performance.py ostrich/low-p-query.txt cobra_opt/low-p-query.txt 8
python make_mean_line_plot.py ostrich/low-p-query.txt cobra_opt/low-p-query.txt 8

python calc_query_performance.py ostrich/high-p-query.txt cobra_opt/high-p-query.txt 8
python make_mean_line_plot.py ostrich/high-p-query.txt cobra_opt/high-p-query.txt 8

python calc_average.py
python calc_average_mean_line.py

python process_ingestion.py ostrich/ostrich.txt ostrich_opt/ostrich_opt.txt fixup/pre_fix_up_ostrich_opt.txt fixup/fix_up_ostrich_opt.txt cobra_opt/cobra_opt.txt

