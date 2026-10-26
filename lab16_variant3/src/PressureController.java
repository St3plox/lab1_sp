public abstract class PressureController {
    public final String createSignals(double pressure) {
        if (!Double.isFinite(pressure) || pressure < 0) {
            throw new IllegalArgumentException("Давление должно быть конечным и неотрицательным.");
        }
        return calculateSignals(pressure);
    }

    protected abstract String calculateSignals(double pressure);
}
