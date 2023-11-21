#include "lpsxxx.h"
#include "lpsxxx_params.h"
#include "xtimer.h"

static lpsxxx_t lpsxxx;

int main(void) {
	lpsxxx_init(&lpsxxx, &lpsxxx_params[0]);
	while (1) {
	    uint16_t pres = 0;
	    int16_t temp = 0;
	    lpsxxx_read_temp(&lpsxxx, &temp);
	    lpsxxx_read_pres(&lpsxxx, &pres);
	    printf("Pressure: %uhPa, Temperature: %i.%u°C\n",
           	 pres, (temp / 100), (temp % 100));
 	    xtimer_sleep(2);
}

	return 0;
}
