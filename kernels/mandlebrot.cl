
float scale(float valueIn, float origMin, float origMax, float scaledMin, float scaledMax) {
	return ((scaledMax - scaledMin) * (valueIn - origMin) / (origMax - origMin)) + scaledMin;
}

__kernel void mandlebrot (
	global int2* image_res,
  __write_only image2d_t image,
  global float4* range
  ){

  size_t x_pixel = get_global_id(0);
  size_t y_pixel = get_global_id(1);

  int2 pixel = (int2)(x_pixel, y_pixel);

  float x0 = scale(x_pixel, 0, (*image_res).x, (*range).x, (*range).y);
  float y0 = scale(y_pixel, 0, (*image_res).y, (*range).z, (*range).w);

  float x = 0.0;
  float y = 0.0;

  int iteration_count = 0;
  int interation_threshold = 1000;

  while (x*x + y*y < 4 && iteration_count < interation_threshold) {
    float x_temp = x*x - y*y + x0;
    y = 2 * x * y + y0;
    x = x_temp;
    iteration_count++;
  }

  int val = scale(iteration_count, 0, 1000, 0, 16777216);

  write_imagef(image, pixel, (float4)(val & 0xff, (val >> 8) & 0xff, (val >> 16) & 0xff, 200));

  return;

}
