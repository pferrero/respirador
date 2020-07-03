#include <stdio.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "in_out.h"

TaskHandle_t xImprimirAlarmaHandle;
const TickType_t xDelay500ms = pdMS_TO_TICKS( 500UL );
/*--------------------------------------------------------------------------------*/
/*
 * Prototypes for the standard FreeRTOS callback/hook functions implemented
 * within this file.
 */
void vApplicationMallocFailedHook( void );
void vApplicationIdleHook( void );

void vApplicationMallocFailedHook( void )
{
	/* vApplicationMallocFailedHook() will only be called if
	configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
	function that will get called if a call to pvPortMalloc() fails.
	pvPortMalloc() is called internally by the kernel whenever a task, queue,
	timer or semaphore is created.  It is also called by various parts of the
	demo application.  If heap_1.c or heap_2.c are used, then the size of the
	heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
	FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
	to query the size of free heap space that remains (although it does not
	provide information on how the remaining heap might be fragmented). */
	vAssertCalled( __LINE__, __FILE__ );
}

void vAssertCalled( unsigned long ulLine, const char * const pcFileName )
{
 	taskENTER_CRITICAL();
	{
        printf("[ASSERT] %s:%lu\n", pcFileName, ulLine);
        fflush(stdout);
	}
	taskEXIT_CRITICAL();
	exit(-1);
}

void vApplicationIdleHook( void )
{
	/* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
	to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
	task.  It is essential that code added to this hook function never attempts
	to block in any way (for example, call xQueueReceive() with a block time
	specified, or call vTaskDelay()).  If the application makes use of the
	vTaskDelete() API function (as this demo application does) then it is also
	important that vApplicationIdleHook() is permitted to return to its calling
	function, because it is the responsibility of the idle task to clean up
	memory allocated by the kernel to any task that has since been deleted. */

	//metodoALlamarEnIdle();

}
/*--------------------------------------------------------------------------------*/

void vEnviarInfo (void *p) {
	char buff[80];
	BaseType_t xStatus;
	TickType_t xLastWakeTime = xTaskGetTickCount();
	for(;;) {
		sprintf(buff, "niv_pres %d | prox_t %d | curr_t %d \n", nivel_limite_presion, pdMS_TO_TICKS(tiempo_respiracion), xLastWakeTime);
		xStatus = xQueueSend(infoQueue, buff, (TickType_t) 0);
		if( xStatus != pdPASS) {
			printf("Could not send to the queue.\n" );
		}
		// La task se bloquea por exactamente 500ms
		vTaskDelayUntil( &xLastWakeTime, xDelay500ms );
	}
}

void vNotificarAlarma (void *p) {
	TickType_t xLastWakeTime = xTaskGetTickCount();
	for(;;) {
		xTaskNotify(xImprimirAlarmaHandle, 0x1, eSetValueWithOverwrite);
		// La task se bloquea por exactamente 500ms
		vTaskDelayUntil( &xLastWakeTime, xDelay500ms );
	}
}

void vManejarRespiracion (void *p){
	for(;;) {
		if( xSemaphoreTake(xSemaforoRespiracion, 0) == pdPASS ) {
			printf("Semaforo\n");
		}
		switch(nivel_limite_presion) {
		case 0:
			tiempo_respiracion = 1000;
			break;
		case 1:
			tiempo_respiracion = 700;
			break;
		case 2:
			tiempo_respiracion = 500;
			xTaskNotify(xImprimirAlarmaHandle, 0x4, eSetValueWithOverwrite);
			break;
		default:
			printf("Nivel de respiración inválido.\n");
		}
		vTaskDelay(pdMS_TO_TICKS(tiempo_respiracion));
	}
}

int main(void){

	// ... COMPLETAR
	/* Perform any hardware setup necessary. */
	//prvSetupHardware();
	/* xTaskCreate(
					vTask1,  Pointer to the function that implements the task.
					"Task 1", Text name for the task. This is to facilitate debugging only.
					1000, Stack depth - small microcontrollers will use much less stack than this.
					NULL, This example does not use the task parameter.
					1, This task will run at priority 1.
					NULL ); This example does not use the task handle.
	*/
	// Alarmas
	xTaskCreate(vImprimirAlarma, "imprimirAlarma", 200, NULL, tskIDLE_PRIORITY+1, &xImprimirAlarmaHandle);
	xTaskCreate(vNotificarAlarma, "notificarAlarma", 200, NULL, tskIDLE_PRIORITY+1, NULL);
	// Info
	xTaskCreate(vEnviarInfo, "enviarInfo", 200, NULL, tskIDLE_PRIORITY+1, NULL);
	xTaskCreate(vImprimirInfo, "imprimirInfo", 200, NULL, tskIDLE_PRIORITY+1, NULL);
	// Respiración
	xTaskCreate(vRealizarRespiracion, "realizarRespiracion", 200, NULL, tskIDLE_PRIORITY+2, NULL);
	xTaskCreate(vManejarRespiracion, "manejarRespiracion", 200, NULL, tskIDLE_PRIORITY+2, NULL);

	// Valores iniciales
	infoQueue = xQueueCreate(160, 80); // buffer_size, item_size
	char texto_inicial[80];
	sprintf(texto_inicial, "INICIANDO SISTEMA");
	//         queue    , buffer       , cant. max de tick que debe permanecer en blocked hasta que haya espacio en la queue. 0 no espera si el buffer está lleno
	xQueueSend(infoQueue, texto_inicial, (TickType_t) 0);
	xSemaforoRespiracion = xSemaphoreCreateBinary();
	tiempo_respiracion = 1000;

	// Iniciar el Scheduler
	vTaskStartScheduler();

	// Nunca se llega acá
	for(;;);
	return 0;
}
