################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/D_DRIVE_BACKUP/workspace/stm32cubewl-v1-4-0/STM32Cube_FW_WL_V1.4.0/Projects/NUCLEO-WL55JC/Examples_LL/TIM/TIM_PWMOutput_Init/Src/main.c \
C:/D_DRIVE_BACKUP/workspace/stm32cubewl-v1-4-0/STM32Cube_FW_WL_V1.4.0/Projects/NUCLEO-WL55JC/Examples_LL/TIM/TIM_PWMOutput_Init/Src/stm32wlxx_it.c \
../Application/User/syscalls.c \
../Application/User/sysmem.c 

OBJS += \
./Application/User/main.o \
./Application/User/stm32wlxx_it.o \
./Application/User/syscalls.o \
./Application/User/sysmem.o 

C_DEPS += \
./Application/User/main.d \
./Application/User/stm32wlxx_it.d \
./Application/User/syscalls.d \
./Application/User/sysmem.d 


# Each subdirectory must supply rules for building sources it contributes
Application/User/main.o: C:/D_DRIVE_BACKUP/workspace/stm32cubewl-v1-4-0/STM32Cube_FW_WL_V1.4.0/Projects/NUCLEO-WL55JC/Examples_LL/TIM/TIM_PWMOutput_Init/Src/main.c Application/User/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 '-DMBEDTLS_CONFIG_FILE="mbedtls_config.h"' -DHSE_VALUE=8000000 -DSTM32WL55xx -DLSI_VALUE=32000 -DCORE_CM4 -DHSE_STARTUP_TIMEOUT=100 -DHSI_VALUE=16000000 -DLSE_STARTUP_TIMEOUT=5000 -DDEBUG -DLSE_VALUE=32768 -DDATA_CACHE_ENABLE=1 -DVDD_VALUE=3300 -DINSTRUCTION_CACHE_ENABLE=1 -DEXTERNAL_CLOCK_VALUE=48000 -DUSE_FULL_LL_DRIVER -DPREFETCH_ENABLE=0 -c -I../../Inc -I../../../../../../../Drivers/STM32WLxx_HAL_Driver/Inc -I../../../../../../../Drivers/CMSIS/Device/ST/STM32WLxx/Include -I../../../../../../../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Application/User/stm32wlxx_it.o: C:/D_DRIVE_BACKUP/workspace/stm32cubewl-v1-4-0/STM32Cube_FW_WL_V1.4.0/Projects/NUCLEO-WL55JC/Examples_LL/TIM/TIM_PWMOutput_Init/Src/stm32wlxx_it.c Application/User/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 '-DMBEDTLS_CONFIG_FILE="mbedtls_config.h"' -DHSE_VALUE=8000000 -DSTM32WL55xx -DLSI_VALUE=32000 -DCORE_CM4 -DHSE_STARTUP_TIMEOUT=100 -DHSI_VALUE=16000000 -DLSE_STARTUP_TIMEOUT=5000 -DDEBUG -DLSE_VALUE=32768 -DDATA_CACHE_ENABLE=1 -DVDD_VALUE=3300 -DINSTRUCTION_CACHE_ENABLE=1 -DEXTERNAL_CLOCK_VALUE=48000 -DUSE_FULL_LL_DRIVER -DPREFETCH_ENABLE=0 -c -I../../Inc -I../../../../../../../Drivers/STM32WLxx_HAL_Driver/Inc -I../../../../../../../Drivers/CMSIS/Device/ST/STM32WLxx/Include -I../../../../../../../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Application/User/%.o Application/User/%.su Application/User/%.cyclo: ../Application/User/%.c Application/User/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 '-DMBEDTLS_CONFIG_FILE="mbedtls_config.h"' -DHSE_VALUE=8000000 -DSTM32WL55xx -DLSI_VALUE=32000 -DCORE_CM4 -DHSE_STARTUP_TIMEOUT=100 -DHSI_VALUE=16000000 -DLSE_STARTUP_TIMEOUT=5000 -DDEBUG -DLSE_VALUE=32768 -DDATA_CACHE_ENABLE=1 -DVDD_VALUE=3300 -DINSTRUCTION_CACHE_ENABLE=1 -DEXTERNAL_CLOCK_VALUE=48000 -DUSE_FULL_LL_DRIVER -DPREFETCH_ENABLE=0 -c -I../../Inc -I../../../../../../../Drivers/STM32WLxx_HAL_Driver/Inc -I../../../../../../../Drivers/CMSIS/Device/ST/STM32WLxx/Include -I../../../../../../../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Application-2f-User

clean-Application-2f-User:
	-$(RM) ./Application/User/main.cyclo ./Application/User/main.d ./Application/User/main.o ./Application/User/main.su ./Application/User/stm32wlxx_it.cyclo ./Application/User/stm32wlxx_it.d ./Application/User/stm32wlxx_it.o ./Application/User/stm32wlxx_it.su ./Application/User/syscalls.cyclo ./Application/User/syscalls.d ./Application/User/syscalls.o ./Application/User/syscalls.su ./Application/User/sysmem.cyclo ./Application/User/sysmem.d ./Application/User/sysmem.o ./Application/User/sysmem.su

.PHONY: clean-Application-2f-User

