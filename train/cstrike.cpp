#include <math.h>
#include "ann.h"

int main(void)
{
	ann::uint_vec layout;
	ann nn;

	ann::double_vec data(4);
	ann::double_vec output(1);

	layout.push_back(4);
	layout.push_back(5);
	layout.push_back(3);
	layout.push_back(1);
	
	ann::train_data d;
	ann::train_set t, v;

	FILE *fp;

	nn.initialize(layout);

	fp = fopen("data", "r");

	int i = 0;
	while (1) {
		if (fscanf(fp, "%lf %*f %lf %lf %lf %lf", &data[0], &data[1], &data[2], &data[3], &output[0]) != 5)
			break;

		for (int j = 0; j < data.size(); j++)
			data[j] = log(data[j] + 1);

		d = ann::train_data(data, output);

		i++;

		if ((i % 10) == 0)
			v.push_back(d);
		else
			t.push_back(d);
	}
	
	fclose(fp);

	printf("loaded %d samples\n", i);

	double limit = 0.8;
	
	bool done = false;
	size_t k = 1;
	while (!done) {
		nn.train(t);

		size_t i, r1 = 0, r2 = 0;

		for (i = 0; i < t.size(); i++) {
			nn.calculate(t[i].first);
			nn.get_output(output);

			if ((t[i].second)[0] == 1 && output[0] >= 0.5)
				r1++;

			if ((t[i].second)[0] == 0 && output[0] < 0.5)
				r1++;
		}
		
		for (i = 0; i < v.size(); i++) {
			nn.calculate(v[i].first);
			nn.get_output(output);

			if ((v[i].second)[0] == 1 && output[0] >= 0.5)
				r2++;

			if ((v[i].second)[0] == 0 && output[0] < 0.5)
				r2++;
		}

		double rp1 = (double)r1 / t.size();
		double rp2 = (double)r2 / v.size();

		double rp = (rp1 + rp2) / 2;

		if (rp >= 0.95)
			done = true;

		printf("\r%lu %lf (%lf %lf)", k++, rp, rp1, rp2);
		fflush(stdout);

		if (rp >= limit) {
			char buff[64];
			
			sprintf(buff, "aa_cstrike%d.nn", k - 1);
			nn.save(buff);

			limit += 0.005;

			printf("\nsaved @%lf, new limit is %lf\n", rp, limit);
		}
	}

	printf("\n");
	nn.save("aa_cstrike.nn");
/*
  	nn.load("aa_cs.nn");

	fp = fopen("data", "r");

	int i = 0;
	int k = 0;

	double o;
	
	while (1) {
		if (fscanf(fp, "%lf %lf %lf %lf %lf %lf", &data[0], &data[1], &data[2], &data[3], &data[4], &o) != 6)
			break;

		nn.calculate(data);
		nn.get_output(output);

		if (o == 1)
			if (output[0] >= 0.5)
				k++;

		if (o == 0)
			if (output[0] < 0.5)
				k++;

		i++;
	}

	printf("%lf\n", (double)k / i);
	
	fclose(fp);
*/	
}
