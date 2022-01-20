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
all: lpcxpresso812_NTAG_THMS.axf

# Tool invocations
lpcxpresso812_NTAG_THMS.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -Xlinker -Map="lpcxpresso812_NTAG_THMS.map" -Xlinker --gc-sections -Xlinker -print-memory-usage -Xlinker --sort-section=alignment -Xlinker --cref -mcpu=cortex-m0plus -mthumb -T "lpcxpresso812_NTAG_THMS_Debug.ld" -o "lpcxpresso812_NTAG_THMS.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) lpcxpresso812_NTAG_THMS.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "lpcxpresso812_NTAG_THMS.axf"; # arm-none-eabi-objcopy -v -O binary "lpcxpresso812_NTAG_THMS.axf" "lpcxpresso812_NTAG_THMS.bin" ; # checksum -p LPC812 -d "lpcxpresso812_NTAG_THMS.bin";
	-@echo ' '

.PHONY: all clean dependents post-build

-include ../makefile.targets
