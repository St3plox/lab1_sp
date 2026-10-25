import java.util.Locale;

/** Клапанный задатчик с фиксированной суммой напряжений 10 В. */
public class ValvePressureController extends PressureController {
    private final double inputPressure;
    private final double outputPressure;

    public ValvePressureController(double inputPressure, double outputPressure) {
        if (!Double.isFinite(inputPressure) || !Double.isFinite(outputPressure)
                || outputPressure < 0 || inputPressure <= outputPressure) {
            throw new IllegalArgumentException("Требуется Pin > Pout >= 0, оба значения конечные.");
        }
        this.inputPressure = inputPressure;
        this.outputPressure = outputPressure;
    }

    @Override
    protected String calculateSignals(double pressure) {
        if (pressure < outputPressure || pressure > inputPressure) {
            throw new IllegalArgumentException("Давление вне диапазона [Pout, Pin].");
        }
        //  P = (U1 * Pin + U2 * Pout) / (U1 + U2), где U1 + U2 = 10.
        var u1 = 10.0 * ((pressure - outputPressure) / (inputPressure - outputPressure));
        var u2 = 10.0 - u1;
        return String.format(Locale.ROOT, "Клапанный: U1 = %.6f В, U2 = %.6f В", u1, u2);
    }
}
