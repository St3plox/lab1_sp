public class PressPressureController extends PressureController {
    private final double initialPressure;
    private final double initialVolume;
    private final double coefficient;

    public PressPressureController(double initialPressure, double initialVolume, double coefficient) {
        if (!Double.isFinite(initialPressure) || initialPressure < 0
                || !Double.isFinite(initialVolume) || initialVolume <= 0
                || !Double.isFinite(coefficient) || coefficient <= 0) {
            throw new IllegalArgumentException("Требуется P0 >= 0, V0 > 0, k > 0; значения конечные.");
        }
        this.initialPressure = initialPressure;
        this.initialVolume = initialVolume;
        this.coefficient = coefficient;
    }

    @Override
    protected String calculateSignals(double pressure) {
        var volume = ((initialPressure + 1.0) / (pressure + 1.0)) * initialVolume;
        var displacement = initialVolume - volume;
        var signal = Math.signum(displacement);
        var duration = Math.abs(displacement) / coefficient;
        if (!Double.isFinite(volume) || volume <= 0 || !Double.isFinite(duration)) {
            throw new IllegalArgumentException("Параметры выходят за числовой диапазон расчёта.");
        }
        return String.format("Прессовый: f = %.0f, dt = %.6f с, V = %.6f л", signal, duration, volume);
    }
}
