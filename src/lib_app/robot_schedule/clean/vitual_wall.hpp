#include "apptypes.hpp"
#include "map.hpp"

Rectangle generate_vitual_area(Point leftBottom,Point rightBottom,Point rightTop,Point leftTop);
int judge_point_in_rec(Point p,Rectangle rec);
int judge_point_out_rec(Point p,Rectangle rec);
Rectangle generate_charge_area();
void generate_area_path();
Rectangle generate_clean_zone(Point now, Point Diagonal_point);
void generate_forrbiden_map(Rectangle zone);