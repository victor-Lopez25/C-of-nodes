## Requerimiento

En Hyperloop UPV en firmware hemos diseñado un sistema para dar errores de compilación si usamos periféricos de forma incorrecta y que también permite algún nivel de detección automática de periféricos:

Yendo desde lo más simple, podemos declarar un GPIO que en STMicroelectronics tiene un puerto (puerto A - H), pin (número), modo de operación, pull (pull up, down, none), speed (low, med, high, very high) y una función alterna (número).

Un puerto y pin como el F6 solo permite algunas funciones alternas:
```c++
// en un código autogenerado
constexpr GPIODomain::Pin PF6{F, GPIO_PIN_6, 0b0110011110100101 /* estas son las funciones alternas permitidas */};


// en código del usuario
constexpr GPIODomin::GPIO gpio_PF6_def{PF6, OperationMode::INPUT, Pull::None, Speed::Low, AlternateFunction::AF14};
/* Esto dará error de compilación porque el AlternateFunction no es correcto para este pin. En la práctica el AlternateFunction no se pone manualmente si no que el periférico lo pone por ti dependiendo de lo que quieras, por ejemplo un pwm (Pulse Width Modulation) lo declaras desde un timer y te puede dar error porque ese timer no permite una pwm con ese pin que le hayas dado.
*/
```

Otro ejemplo un poco más complejo, timers:

Intentamos declarar un timer sin importarnos cual usar pero no queremos colisionar con otros usados que sí requerimos un timer específico (por pines o por características de timers):
```cpp
// pedimos un timer 32 bit, si no quedan nos dará error de compilación
constexpr ST_LIB::TimerDomain::Timer timer_any_32bit_def{{
  .request = ST_LIB::TimerRequest::Any32bit,
}};

// pedimos cualquier timer genérico, usará primero los timers más frecuentes como de 16 bit antes de los de 32 bit
constexpr ST_LIB::TimerDomain::Timer timer_any_general_purpose_def{{
  .request = ST_LIB::TimerRequest::AnyGeneralPurpose,
}};

// pedimos el timer 23 específicamente, ya que este es específico se reservará antes que los anteriores (no importa el órden de definición)
constexpr ST_LIB::TimerDomain::Timer timer23_def{{
  .request = ST_LIB::TimerRequest::GeneralPurpose32bit_23,
}};

// si pedimos el mismo timer 2 veces también da error de compilación
constexpr ST_LIB::TimerDomain::Timer timer23_def_2{{
  .request = ST_LIB::TimerRequest::GeneralPurpose32bit_23,
}};
```

Veamos el ejemplo de crear una pwm con un timer que puede dar un error de compilación:
```c++
// creamos un pin para una pwm, aquí no da error ya que solo es un dato
constexpr ST_LIB::TimerPin PWM_pin {
  .af = ST_LIB::TimerAF::PWM,
  .pin = ST_LIB::PE9,
  .channel = ST_LIB::TimerChannel::CHANNEL_1,
};

inline constexpr ST_LIB::TimerDomain::Timer tim_decl{{
  // error de compilación: el pin PE9 no se puede configurar como pwm con canal 1 con el timer13
  .request = ST_LIB::TimerRequest::GeneralPurpose_13,

  // el timer1 es el correcto en este caso
  // .request = ST_LIB::TimerRequest::Advanced_1,
}, PWM_pin};

// más tarde podemos crear un pwm desde este timer y el pin
```

Y aquí un ejemplo de uso completo (un led que parpadea):
```c++
#include "ST-LIB.hpp"

constexpr ST_LIB::DigitalOutputDomain::DigitalOutput led_def{ST_LIB::PG8};

/* necesitamos poner todos los objetos de dominios en el board */
using exampleBoard = ST_LIB::Board<led_def>;

int main() {
  exampleBoard::init();

  /* obtenemos el objeto runtime desde la definición del objeto compile time */
  auto led = &exampleBoard::instance_of<led_def>();
  while(1) {
    led->toggle();
    HAL_Delay(200);
  }
}
```

Hay más ejemplos en nuestra librería, la ST-LIB de este tipo de metaprogramación. Notablemente, un "Dominio" puede depender de otro, puede que no se vea muy claro con los timers y los GPIOs pero hay formas más complejas de esto como un dominio que depende de múltiples dominios donde alguno depende de otros también.

Todo esto es posible porque utilizamos un "objeto" global que tiene que dar buffers constexpr entre todos los objetos de la ST-LIB que permitan comprobar su uso en tiempo de compilación. Cosa que no es para nada cómoda de utilizar.

En mi opinión este "objeto" global al que llamamos un ST_LIB::Board, no debería de ser necesario, dado que tenemos ya un concepto que maneja todos los tiempos de compilación, el compilador. El compilador debería de dar las facilidades para permitir hacer ésto pero sin necesidad de un objeto global o usando un objeto global más simple.

Otra cosa que es mucho más simple pero no la he visto en ningún lenguaje todavía es permitir hacer errores de compilación con formateo, en C por ejemplo tienes el `#error` pero es un preprocesador y si lo pones en medio de una función `consteval` o un `if constexpr` da igual si se llama o no, y da igual si la condición es correcta o no, el `#error` se evalúa, entonces da error siempre. Para evitar esto en el Hyperloop usamos una función externa que nunca definimos: `extern void compile_error(const char *msg);` el problema de esto es que para que se vea bien en el output el mensaje debe ser un string literal, pero molesta más no poder formatearlo porque no podemos hacer mensajes de error muy descriptivos.

## Propuesta sintáxis

Esta es una sintaxis que me he inventado para esto y probablemente la real sería algo distinta...

```
import "comptime"

// los comptime.define pueden ir en orden como las definiciones de variables normalmente, entonces podemos tener definido en un import cosas también que estarían en el namespace
comptime.define {
  PF6 := GPIODomain.Pin{Port.F, GPIO_PIN_6, 0b0110011110100101};
  // el resto de los pines...
};

// los periféricos (código de usuario)
comptime.define {
  gpio_PF6 := GPIODomain.GPIO{PF6, OperationMode.INPUT, Pull.None, Speed.Low, AlternateFunction.AF14};

  timer_any_32bit := ST_LIB.TimerDomain.TimerDef{
    .request = ST_LIB.TimerRequest.Any32bit,
  };

  // etc.
};

// para los timers (esto estaría en un import em la st-lib)
comptime.run {
  timers := comptime.get_all_objects(ST_LIB.TimerDomain.TimerDef);

  if len(timers) > ST_LIB.TimerDomain.MAX_TIMERS {
    comptime.errorf("Too many timers requested. Maximum %d, wanted %d", ST_LIB.TimerDomain.MAX_TIMERS, len(timers));
  }

  for tim in timers {
    // reserva timers, comprueba si sus pines están bien, etc.

    // añade una definición de una variable para runtime para el scope de fuera del comptime.run
    comptime.add_runtime_def(ST_LIB.TimerDomain.Timer{ /* ... */ }, tim.name);
  }
};
```
