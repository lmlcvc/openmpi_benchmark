import csv
import os
import re
import numpy as np
import pandas as pd
from matplotlib import pyplot as plt

header_phase = ["timestamp", "comm_type", "message_size", "message_count", "phase", "ru", "bu", "ru_host", "bu_host",
                "avg_rtt", "throughput", "throughput_with_barrier", "errors"]
header_tp = ["timestamp", "comm_type", "message_size", "throughput"]

plot_directories = {
    'nodes': 'plots/plots_per_node',
    'throughput': 'plots/plots_throughput'
}

def clean_csv(directory, new_directory, filename, log_type="phase"):
    header = header_phase if log_type == "phase" else header_tp

    with open(os.path.join(new_directory, filename), 'w', newline='') as cleaned_file:
        writer = csv.DictWriter(cleaned_file, fieldnames=header)
        writer.writeheader()

        with open(os.path.join(directory, filename), 'r', newline='') as original_file:
            reader = csv.DictReader(original_file)
            for row in reader:
                if len(row) == len(header_phase):
                    writer.writerow(row)


def tp_per_phase(dfs_list):
    average_throughputs_per_phase = []

    all_phases = set()

    for _, df in dfs_list.items():
        phases = df['phase'].unique()
        
        all_phases.update(phases)
        
        average_throughput_per_phase = df.groupby('phase')['throughput'].mean()
        average_throughputs_per_phase.append(average_throughput_per_phase)

    all_phases = sorted(list(all_phases))

    bar_width = 0.1
    phase_positions = range(len(all_phases))

    plt.figure(figsize=(12, 6))

    for i, avg_throughput_per_phase in enumerate(average_throughputs_per_phase):
        avg_throughput_values = [avg_throughput_per_phase.get(phase, 0) for phase in all_phases]
        phase_positions_shifted = [pos + i * bar_width for pos in phase_positions]
        plt.bar(phase_positions_shifted, avg_throughput_values, width=bar_width, label=f'{(i+2)} hosts')

    for phase, tp in zip(all_phases, avg_throughput_per_phase):
        plt.text(phase + 0.5, tp + 1200, str(int(phase)), ha='center', va='bottom')

    plt.xticks([])
    plt.title('Average Throughput per Phase')
    plt.xlabel('Phase')
    plt.ylabel('Average Throughput [Mbit/s]')
    plt.legend(loc='upper left', bbox_to_anchor=(1, 1))

    plot_directory = plot_directories['throughput']
    if not os.path.exists(plot_directory):
        os.makedirs(plot_directory)
    plt.savefig(os.path.join(plot_directory, 'average_throughput_per_phase.png'))


def tp_through_time(df, number, column='throughput'):
    phases = df['phase'].unique()
    min_entries = min(df.groupby('phase')['timestamp'].count())  # Find the phase with the least entries

    plt.figure(figsize=(12, 6))
    ax = plt.subplot(111)

    for phase in phases:
        phase_data = df[df['phase'] == phase][:min_entries]  # Truncate or pad to match the phase with the least entries
        ax.plot(range(len(phase_data)), phase_data[column], label=f'Phase {phase}')

    plt.xlabel('Entry')
    plt.ylabel('Throughput [Mbit/s]')
    plt.title('Throughput Through Time')
    plt.legend(loc='upper left', bbox_to_anchor=(1, 1))
    plt.xticks(rotation=45)

    plot_directory = plot_directories['nodes']
    node_directory = os.path.join(plot_directory, str(number))
    if not os.path.exists(plot_directory):
        os.makedirs(plot_directory)
    if not os.path.exists(node_directory):
        os.makedirs(node_directory)
    plt.savefig(os.path.join(node_directory, f'{column}.png'))



def tp_nodes_over_time(dfs_dict):
    plt.figure(figsize=(10, 6))
    ax = plt.subplot(111)

    min_length = min(len(df) for df in dfs_dict.values())

    for nodes, df in dfs_dict.items():
        truncated_df = df.head(min_length)
        tp_data = truncated_df['throughput']
        ax.plot(range(len(tp_data)), tp_data, label=nodes, linewidth=0.5)

    plt.xlabel('Entry')
    plt.ylabel('Throughput [Mbit/s]')
    plt.title('Throughput through time by number of nodes')
    plt.legend(loc='upper left', bbox_to_anchor=(1, 1))

    plot_directory = plot_directories['throughput']
    if not os.path.exists(plot_directory):
        os.makedirs(plot_directory)
    plt.savefig(os.path.join(plot_directory, 'throughput_through_time.png'))


def tp_nodes_bar(dfs_dict):
    average_throughputs = {}
    min_length = min(len(df) for df in dfs_dict.values())

    for number, df in dfs_dict.items():
        truncated_df = df.head(min_length)
        average_throughput = truncated_df['throughput'].mean()
        average_throughputs[number] = average_throughput

    nodes = list(average_throughputs.keys())
    average_tp_values = list(average_throughputs.values())

    plt.figure(figsize=(10, 6))
    plt.bar(nodes, average_tp_values, width=0.5)
    plt.xlabel('Number of Nodes')
    plt.ylabel('Average Throughput [Mbit/s]')
    plt.xticks(nodes)
    plt.title('Average Throughput by Number of Nodes')

    for node, tp in zip(nodes, average_tp_values):
        plt.text(node, tp + 1000, str(int(tp)), ha='center', va='bottom')

    plot_directory = plot_directories['throughput']
    if not os.path.exists(plot_directory):
        os.makedirs(plot_directory)
    plt.savefig(os.path.join(plot_directory, 'average_throughput_by_number_of_nodes.png'))



if __name__ == '__main__':
    phase_dfs = {}
    for file in os.listdir('uns/'):
        if file.startswith('phase'):
            match = re.search(r'_(\d+)\.csv', file)
            number = int(match.group(1)) if match else None

            try:
                phase_df = pd.read_csv(os.path.join('/runs', file), header=0)
            except pd.errors.ParserError:
                print(file)
            phase_df = phase_df[phase_df['errors'] == 0]
            phase_dfs[number] = phase_df

            tp_through_time(phase_df, number)
            tp_through_time(phase_df, number, 'throughput_with_barrier')

    sorted_phase_dfs = {key: phase_dfs[key] for key in sorted(phase_dfs.keys())}
    tp_per_phase(sorted_phase_dfs)

    throughput_dfs = {}
    for file in os.listdir('runs'):
        if file.startswith('throughput'):
            match = re.search(r'_(\d+)\.csv', file)
            number = int(match.group(1)) if match else None

            throughput_df = pd.read_csv(os.path.join('runs', file), header=0)
            throughput_dfs[number] = throughput_df

    sorted_throughput_dfs = {key: throughput_dfs[key] for key in sorted(throughput_dfs.keys())}

    tp_nodes_over_time(sorted_throughput_dfs)
    tp_nodes_bar(sorted_throughput_dfs)
