################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Lib/Src/font.c \
../Lib/Src/lm75.c \
../Lib/Src/oled.c \
../Lib/Src/user_rtc.c 

OBJS += \
./Lib/Src/font.o \
./Lib/Src/lm75.o \
./Lib/Src/oled.o \
./Lib/Src/user_rtc.o 

C_DEPS += \
./Lib/Src/font.d \
./Lib/Src/lm75.d \
./Lib/Src/oled.d \
./Lib/Src/user_rtc.d 


# Each subdirectory must supply rules for building sources it contributes
Lib/Src/%.o Lib/Src/%.su Lib/Src/%.cyclo: ../Lib/Src/%.c Lib/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F411xE -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../App/Inc -I../Lib/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Lib-2f-Src

clean-Lib-2f-Src:
	-$(RM) ./Lib/Src/font.cyclo ./Lib/Src/font.d ./Lib/Src/font.o ./Lib/Src/font.su ./Lib/Src/lm75.cyclo ./Lib/Src/lm75.d ./Lib/Src/lm75.o ./Lib/Src/lm75.su ./Lib/Src/oled.cyclo ./Lib/Src/oled.d ./Lib/Src/oled.o ./Lib/Src/oled.su ./Lib/Src/user_rtc.cyclo ./Lib/Src/user_rtc.d ./Lib/Src/user_rtc.o ./Lib/Src/user_rtc.su

.PHONY: clean-Lib-2f-Src

