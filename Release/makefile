################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include utilities/subdir.mk
-include timer/subdir.mk
-include startup/subdir.mk
-include source/subdir.mk
-include drivers/subdir.mk
-include device/subdir.mk
-include component/uart/subdir.mk
-include board/subdir.mk
-include THMS/subdir.mk
-include NTAG/subdir.mk
-include NDEF_Parse/subdir.mk
-include ADS1115/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: NFC-THMS-Sensor-Tag.axf

# Tool invocations
NFC-THMS-Sensor-Tag.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -Xlinker -Map="NFC-THMS-Sensor-Tag.map" -Xlinker --gc-sections -Xlinker -print-memory-usage -Xlinker --sort-section=alignment -Xlinker --cref -mcpu=cortex-m0plus -mthumb -T "NFC-THMS-Sensor-Tag_Release.ld" -o "NFC-THMS-Sensor-Tag.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) NFC-THMS-Sensor-Tag.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "NFC-THMS-Sensor-Tag.axf"; # arm-none-eabi-objcopy -v -O binary "NFC-THMS-Sensor-Tag.axf" "NFC-THMS-Sensor-Tag.bin" ; # checksum -p LPC812 -d "NFC-THMS-Sensor-Tag.bin";
	-@echo ' '

.PHONY: all clean dependents post-build

-include ../makefile.targets
