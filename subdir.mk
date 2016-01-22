# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
$(DIR_SRC)/config.c \
$(DIR_SRC)/data.c \
$(DIR_SRC)/export.c \
$(DIR_SRC)/fwatch.c \
$(DIR_SRC)/import.c \
$(DIR_SRC)/log.c \
$(DIR_SRC)/nvipfix.c \
$(DIR_SRC)/types.c 

OBJS += \
$(DIR_OBJ)/config.o \
$(DIR_OBJ)/data.o \
$(DIR_OBJ)/export.o \
$(DIR_OBJ)/fwatch.o \
$(DIR_OBJ)/import.o \
$(DIR_OBJ)/log.o \
$(DIR_OBJ)/nvipfix.o \
$(DIR_OBJ)/types.o 

C_DEPS += \
$(DIR_DEP)/config.d \
$(DIR_DEP)/data.d \
$(DIR_DEP)/export.d \
$(DIR_DEP)/fwatch.d \
$(DIR_DEP)/import.d \
$(DIR_DEP)/log.d \
$(DIR_DEP)/nvipfix.d \
$(DIR_DEP)/types.d 


# Each subdirectory must supply rules for building sources it contributes
$(DIR_OBJ)/%.o: $(DIR_SRC)/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	$(CC) -std=c99 `pkg-config --cflags glib-2.0` -O0 -g3 -Wall -c -fmessage-length=0 -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
