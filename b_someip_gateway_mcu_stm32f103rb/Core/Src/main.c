#include "main.h"

int main(void)
{
	// 1. System Init (Module , Peripherals ...)
	system_init();

	// 2. Create Task
	xTaskCreate((TaskFunction_t)f_inittask,
				"inittask",
				BKEL_TASK_STACK_SIZE_MIN,
				NULL, BKEL_TASK_PRI_NORMAL_0, NULL);

	// 3. Start Scheduler
	vTaskStartScheduler();

	for(;;){}
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
