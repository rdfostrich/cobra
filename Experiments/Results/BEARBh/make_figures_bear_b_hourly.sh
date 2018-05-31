#!/bin/bash
cd /media/thibault/278A4FA0479795D7/final/BEARBh/
python calc_query_performance.py ostrich/p-query.txt cobra_opt/p-query.txt 400
python make_mean_line_plot.py ostrich/p-query.txt cobra_opt/p-query.txt 400
python calc_query_performance.py ostrich/po-query.txt cobra_opt/po-query.txt 400
python make_mean_line_plot.py ostrich/po-query.txt cobra_opt/po-query.txt 400

python calc_average.py
python calc_average_mean_line.py

python process_ingestion.py ostrich/ostrich.txt ostrich_opt/ostrich_opt.txt fixup/pre_fix_up_ostrich_opt.txt fixup/fix_up_ostrich_opt.txt cobra_opt/cobra_opt.txt

