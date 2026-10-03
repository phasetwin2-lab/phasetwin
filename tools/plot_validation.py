"""Plot actual C++ test output. Optional dependency: matplotlib."""
from pathlib import Path
import argparse
import csv
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

parser = argparse.ArgumentParser()
parser.add_argument('--data-dir', type=Path, default=Path('.'))
args = parser.parse_args()
with (args.data_dir / 'alignment_waveforms.csv').open() as f:
    rows = [{key: float(value) for key, value in row.items()} for row in csv.DictReader(f)]
rows = [row for row in rows if row['time_ms'] <= 5]
t = [row['time_ms'] for row in rows]
fig, axes = plt.subplots(2, 1, figsize=(10, 5.2), sharex=True, sharey=True)
fig.patch.set_facecolor('#121820')
for ax, fields, title in zip(axes, [('raw_a', 'raw_b'), ('corrected_a', 'delayed_b')],
                            ['BEFORE: A delayed by 336.37 samples and inverted',
                             'AFTER: measured correction applied by the tested audio engine']):
    ax.set_facecolor('#19232d')
    ax.plot(t, [row[fields[0]] for row in rows], color='#72e9c6', linewidth=1.4, label='A')
    ax.plot(t, [row[fields[1]] for row in rows], color='#ffb86b', linewidth=1.1, linestyle='--', label='B')
    ax.set_title(title, color='white', loc='left', fontsize=11)
    ax.set_ylabel('Amplitude', color='#b5bcc9')
    ax.tick_params(colors='#b5bcc9')
    ax.grid(alpha=.15)
    for spine in ax.spines.values(): spine.set_color('#344351')
    ax.legend(facecolor='#19232d', labelcolor='white', edgecolor='#344351', loc='upper right')
axes[-1].set_xlabel('Time (ms) — 48 kHz', color='#b5bcc9')
fig.suptitle('PhaseTwin 1.1 — real DSP regression output (not a plugin screenshot)', color='white', fontsize=12)
fig.tight_layout()
fig.savefig(args.data_dir / 'alignment_validation.png', dpi=150, facecolor=fig.get_facecolor())
