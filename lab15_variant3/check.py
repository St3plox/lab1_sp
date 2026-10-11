"""Проверки правильности подсчёта и сохранения параметров, в отдельном каталоге."""
import csv
import os
from pathlib import Path
import subprocess
import tempfile

PROGRAM = str(Path(__file__).with_name("points").resolve())


def check(rows, cols, threads, distance, coordinates, expected):
    result = subprocess.run(
        [PROGRAM, "--manual", str(rows), str(cols), str(threads), str(distance)],
        input=coordinates, text=True, capture_output=True, check=True,
    )
    assert f"Последовательный результат: {expected}\n" in result.stdout, result.stdout
    assert f"Параллельный результат: {expected}\n" in result.stdout, result.stdout
    with open("results/timings.csv", newline="") as file:
        saved = list(csv.DictReader(file))
    assert len(saved) == 1
    r = saved[0]
    assert (int(r["rows"]), int(r["cols"]), int(r["threads"])) == (rows, cols, threads)
    assert float(r["distance"]) == distance
    assert int(r["serial_count"]) == int(r["parallel_count"]) == expected
    assert r["input_mode"] == "manual" and r["seed"] == ""
    assert r["timestamp"] and int(r["repeats"]) == 7


def main():
    for threads in (1, 2):
        check(2, 4, threads, 5, "3 4 6 0 0 0 -4 -4", 2)
    for threads in (1, 2, 3, 5):
        check(5, 2, threads, 0, "0 0 1 0 0 1 -1 0 0 -1", 4)
    check(1, 2, 1, 10, "0 0", 0)
    check(1, 2, 1, 1, "2 0", 1)
    with open("results/runs.csv", newline="") as file:
        history = list(csv.DictReader(file))
    assert len(history) == 8  # Предыдущие запуски не затёрлись.
    assert history[0]["threads"] == "1" and history[1]["threads"] == "2"
    subprocess.run([PROGRAM, "5", "4", "3", "12.5", "3"], check=True, capture_output=True)
    with open("results/timings.csv", newline="") as file:
        r = next(csv.DictReader(file))
    assert r["threads"] == "3" and r["distance"] == "12.5" and r["repeats"] == "3"
    assert r["input_mode"] == "generated" and r["seed"] == "12345"

    for args, text in [
        (["2", "3", "1", "5"], ""),
        (["2", "4", "3", "5"], ""),
        (["2", "4", "1", "-1"], ""),
        (["0", "4", "1", "5"], ""),
        (["2", "4", "1", "nan"], ""),
        (["2", "4", "1", "5", "102"], ""),
        (["--manual", "1", "2", "1", "5"], "nan 0"),
        (["--manual", "1", "2", "1", "5"], "oops"),
    ]:
        result = subprocess.run([PROGRAM, *args], input=text, text=True, capture_output=True)
        assert result.returncode != 0, args
    print("Проверки прошли: подсчёт, параметры CSV, история запусков, неверный ввод.")


# Проверки не меняют сохранённые результаты пользователя.
with tempfile.TemporaryDirectory() as directory:
    previous = os.getcwd()
    try:
        os.chdir(directory)
        main()
    finally:
        os.chdir(previous)
