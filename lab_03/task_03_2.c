#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(void)
{
	double eps;
	double x;
	double s;
	double a;
	double f;
	int i;
	double delta_abs, delta_rel;
	
	printf("Введите Х:\n");

	if (scanf("%lf", &x) != 1 || x > 1.0 || x < -1.0)
	{
		printf("Некорректный ввод X (Х должен быть от -1 до 1)\n");
		return EXIT_FAILURE;
	}

	printf("Введите точность:\n");

	if (scanf("%lf", &eps) != 1 || eps > 1.0 || eps <= 0.0)
	{
		printf("Некорректный ввод точности (точность должна быть от 0 до 1)\n");
		return EXIT_FAILURE;
	}

	f = asin(x);
	i = 1;
	a = x;
	s = a;

	while (fabs(a) > eps)
	{
		a = (a * i * i * x * x) / ((i + 1) * (i + 2));
		i += 2;
		s += a;
	}

	delta_abs = fabs(f - s);
	delta_rel = delta_abs / fabs(f);
	printf("%.6f %.6f %.6f %.6f\n", s, f, delta_abs, delta_rel);

	return EXIT_SUCCESS;
}
