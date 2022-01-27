################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../board/board.c \
../board/clock_config.c \
../board/pin_mux.c 

OBJS += \
./board/board.o \
./board/clock_config.o \
./board/pin_mux.o 

C_DEPS += \
./board/board.d \
./board/clock_config.d \
./board/pin_mux.d 


# Each subdirectory must supply rules for building sources it contributes
board/%.o: ../board/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -DCPU_LPC812M101JDH20 -DCPU_LPC812M101JDH20_cm0plus -DCPU_LPC812 -DFSL_SDK_ENABLE_I2C_DRIVER_TRANSACTIONAL_APIS=0 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -DSDK_DEBUGCONSOLE=1 -D__REDLIB__ -I"C:\MCUXpresso_Workspace\LPC812_NFC\NFC-THMS-Sensor-Tag\board" -I"C:\MCUXpresso_Workspace\LPC812_NFC\NFC-THMS-Sensor-Tag\THMS" -I"C:\MCUXpresso_Workspace\LPC812_NFC\NFC-THMS-Sensor-Tag\timer" -I"C:\MCUXpresso_Workspace\LPC812_NFC\NFC-THMS-Sensor-Tag\ADS1115" -I"C:\MCUXpresso_Workspace\LPC812_NFC\NFC-THMS-Sensor-Tag\NDEF_Parse" -I"C:\MCUXpresso_Workspace\LPC812_NFC\NFC-THMS-Sensor-Tag\NTAG" -I"C:\MCUXpresso_Workspace\LPC812_NFC\NFC-THMS-Sensor-Tag\source" -I"C:\MCUXpresso_Workspace\LPC812_NFC\NFC-THMS-Sensor-Tag\utilities" -I"C:\MCUXpresso_Workspace\LPC812_NFC\NFC-THMS-Sensor-Tag\drivers" -I"C:\MCUXpresso_Workspace\LPC812_NFC\NFC-THMS-Sensor-Tag\device" -I"C:\MCUXpresso_Workspace\LPC812_NFC\NFC-THMS-Sensor-Tag\component\uart" -I"C:\MCUXpresso_Workspace\LPC812_NFC\NFC-THMS-Sensor-Tag\CMSIS" -Os -fno-common -g -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmacro-prefix-map="../$(@D)/"=. -mcpu=cortex-m0plus -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


