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
		sprintf(buff, "niv_pres %d | prox_t %4d | curr_t %d", nivel_limite_presion, pdMS_TO_TICKS(tiempo_respiracion), xLastWakeTime);
		xStatus = xQueueSend(infoQueue, buff, (TickType_t) 0);
		if( xStatus != pdPASS) {
			printf("Could not send to the queue.\n" );
		}
		// La task se bloquea por exactamente 500ms
		vTaskDelayUntil( &xLastWakeTime, xDelay500ms );
	}
}

// Si una respiración excede los 1.2s se debe generar una alarma
void vNotificarAlarma (void *p) {
	const TickType_t xDelay1200ms = pdMS_TO_TICKS( 1200 );
	for(;;) {
		if( xSemaphoreTake(xSemaforoRespiracion, xDelay1200ms) == pdFALSE ) {
			xTaskNotify(xImprimirAlarmaHandle, 0x2, eSetValueWithOverwrite);
		}
	}

}

// Después de cada respiración realizada, según el nivel de presión se debe ajustar el tiempo de respiración
void vManejarRespiracion (void *p){
	for(;;) {
		switch(nivel_limite_presion) {
		case 0:
			tiempo_respiracion = 1000;
			break;
		case 1:
			tiempo_respiracion = 700;
			break;
		case 2:
			// Si el nivel de presión es 2, además de ajustar el tiempo de respiración, se debe generar una alarma
			tiempo_respiracion = 500;
			xTaskNotify(xImprimirAlarmaHandle, 0x4, eSetValueWithOverwrite);
			break;
		default:
			printf("Nivel de respiración inválido.\n");
		}
		vTaskDelay(pdMS_TO_TICKS(tiempo_respiracion));
	}
}

// Se deben enviar alarmas a display cada 0.5s
void vAlarmaRespiracion (void *p) {
	TickType_t xLastWakeTime = xTaskGetTickCount();
	for(;;) {
		xTaskNotify(xImprimirAlarmaHandle, 0x1, eSetValueWithoutOverwrite);
		// La task se bloquea por exactamente 500ms
		vTaskDelayUntil( &xLastWakeTime, xDelay500ms );
	}
}

int main(void){
	// Alarmas
	xTaskCreate(vImprimirAlarma, "imprimirAlarma", 200, NULL, tskIDLE_PRIORITY+2, &xImprimirAlarmaHandle);
	xTaskCreate(vNotificarAlarma, "notificarAlarma", 200, NULL, tskIDLE_PRIORITY+2, NULL);
	// Info
	xTaskCreate(vEnviarInfo, "enviarInfo", 200, NULL, tskIDLE_PRIORITY+1, NULL);
	xTaskCreate(vImprimirInfo, "imprimirInfo", 200, NULL, tskIDLE_PRIORITY+1, NULL);
	// Respiración
	xTaskCreate(vRealizarRespiracion, "realizarRespiracion", 200, NULL, tskIDLE_PRIORITY+3, NULL);
	xTaskCreate(vManejarRespiracion, "manejarRespiracion", 200, NULL, tskIDLE_PRIORITY+3, NULL);
	xTaskCreate(vAlarmaRespiracion, "alarmaRespiración", 200, NULL, tskIDLE_PRIORITY+2, NULL);

	// Valores iniciales
	infoQueue = xQueueCreate(160, 80); // buffer_size, item_size
	char texto_inicial[80];
	sprintf(texto_inicial, "INICIANDO SISTEMA");
	//         queue    , buffer       , cant. max de tick que debe permanecer en blocked hasta que haya espacio en la queue. 0 no espera si el buffer está lleno
	xQueueSend(infoQueue, texto_inicial, (TickType_t) 0);
	xSemaforoRespiracion = xSemaphoreCreateBinary();
	xSemaphoreGive(xSemaforoRespiracion);
	tiempo_respiracion = 1000;

	// Iniciar el Scheduler
	vTaskStartScheduler();

	// Nunca se llega acá
	for(;;);
	return 0;
}
