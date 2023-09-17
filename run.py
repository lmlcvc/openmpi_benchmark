import os
import pwd
import sys
import signal
import subprocess
import shlex
import argparse
import re

executable_path = "./main"
mpi_base_command = shlex.split('mpirun --oversubscribe')
mpi_environment_options = shlex.split('-x')
mpi_base_options = shlex.split('-bind-to none')
mpi_binding_options = 'hwloc-bind --membind --cpubind os='

# FIXME: add to command:
# hwloc-bind --membind --cpubind os=network device
        # mpirun was unable to find the specified executable file, and therefore did not launch the job.

def signal_handler(sig, frame):
    print('Caught SIGINT. Exiting...')
    # TODO: print total runtime etc
    kill_exit = 0
    sys.exit(kill_exit)

def parse_hostfile(hostfile_name):
    with open(hostfile_name, 'r') as hostfile:
        comment_match = re.compile('([^#]*)')
        matches = map(comment_match.search, hostfile.readlines())
        hosts = [match.group(0).strip() for match in matches if match != None]

    print(hosts)
    return hosts

def start_run(host_list, mode, max_power=None, iterations=None, send_buffer_size=None, receive_buffer_size=None, warmup_iterations=None, message_size=None, ru_buffer_bytes=None, bu_buffer_bytes=None, logging_interval=None, explanation=False, non_blocking=False):
    mpi_command = mpi_base_command.copy()
    mpi_command.extend(mpi_base_options)

    host_info = [(host.split()[0], host.split()[1]) for host in host_list]

    mpi_options = []

    if explanation:
        for (host, network_device) in host_info:
            mpi_command.extend(["--host", f"{host}"])
            mpi_command.extend(["-x", f"UCX_NET_DEVICES={network_device}:1"])
            mpi_command.append(executable_path)
            break
        mpi_command.extend(["-h"])
        return subprocess.Popen(mpi_command, shell=False)

    if warmup_iterations is not None:
        mpi_options.extend(["-w", str(warmup_iterations)])

    if mode == "scan":
        mpi_options.extend(["-s"])
        if max_power is not None:
            mpi_options.extend(["-p", str(max_power)])
        if iterations is not None:
            mpi_options.extend(["-i", str(iterations)])
        if send_buffer_size is not None:
            mpi_options.extend(["-b", str(send_buffer_size)])
        if receive_buffer_size is not None:
            mpi_options.extend(["-r", str(receive_buffer_size)])
        
    elif mode == "fixed":
        mpi_options.extend(["-f"])
        if message_size is not None:
            mpi_options.extend(["-m", str(message_size)])
        if iterations is not None:
            mpi_options.extend(["-i", str(iterations)])
        if ru_buffer_bytes is not None:
            mpi_options.extend(["-r", str(ru_buffer_bytes)])
        if bu_buffer_bytes is not None:
            mpi_options.extend(["-b", str(bu_buffer_bytes)])
        if logging_interval is not None:
            mpi_options.extend(["-l", str(logging_interval)])

    elif mode == "variable":
        mpi_options.extend(["-v"])
        if message_size is not None:
            mpi_options.extend(["-m", str(message_size)])
        if iterations is not None:
            mpi_options.extend(["-i", str(iterations)])
        if ru_buffer_bytes is not None:
            mpi_options.extend(["-r", str(ru_buffer_bytes)])
        if bu_buffer_bytes is not None:
            mpi_options.extend(["-b", str(bu_buffer_bytes)])
        if logging_interval is not None:
            mpi_options.extend(["-l", str(logging_interval)])

    if non_blocking:
        mpi_options.extend(["-n"])

    ru_commands = [executable_path, ' '.join(mpi_options), ":"]
    bu_commands = [executable_path, ' '.join(mpi_options), ":"]

    for (host, network_device) in host_info:
        # RU
        mpi_command.extend(["--host", f"{host}"])
        mpi_command.extend(["-x", f"UCX_NET_DEVICES={network_device}:1"])
        # FIXME: mpi_command.extend([f"{mpi_binding_options}{network_device}:1"])
        mpi_command.extend(ru_commands)

        # BU
        mpi_command.extend(["--host", f"{host}"])
        mpi_command.extend(["-x", f"UCX_NET_DEVICES={network_device}:1"])
        # FIXME: mpi_command.extend([f"{mpi_binding_options}{network_device}:1"])
        mpi_command.extend(bu_commands)
        

    print(f"MPI command: {' '.join(mpi_command)}")
    return subprocess.Popen(mpi_command, shell=False)



def main():
    parser = argparse.ArgumentParser(
        description='EB benchmarking startup script',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument(
        "-H",
        "--hostfile",
        help='Host file.'
        'Every line must contain an hostname. To run multiple units on the same host enter the host name multiple times.'
        ' Lines starting with # are threaded as comments.',
        type=str,
        required=True)
    

    parser.add_argument('-mode', '--mode',
                        type=str,
                        help='Choose a mode of operation: [scan, fixed, variable]',
                        default='fixed',
                        required=True)

    parser.add_argument('-e', '--explanation', action='store_true', help='Print detailed usage explanation')
    parser.add_argument('-n', '--non-blocking', action='store_true', help='Enable nonblocking mode')
    parser.add_argument('-p', '--max-power', type=int, help='Set the maximum power of 2 for message sizes')
    parser.add_argument('-i', '--iterations', type=int, help='Specify the number of iterations')
    parser.add_argument('-bs', '--send-buffer-size', type=int, help='Set the size of the send buffer in messages')
    parser.add_argument('-rs', '--receive-buffer-size', type=int, help='Set the size of the receive buffer in messages')
    parser.add_argument('-m', '--message-size', type=int, help='Set the fixed message size')
    parser.add_argument('-r', '--ru-buffer-bytes', type=int, help='Set the size of the send buffer in bytes')
    parser.add_argument('-b', '--bu-buffer-bytes', type=int, help='Set the size of the receive buffer in bytes')
    parser.add_argument('-w', '--warmup-iterations', type=int, help='Set the number of warmup iterations')
    parser.add_argument('-l', '--logging-interval', type=int, help='Set the interval for average throughput logging in seconds')
    parser.add_argument('-mv', '--message-size-variants', type=int, help='Set the number of message size variants')

    args = parser.parse_args()
    hosts = parse_hostfile(args.hostfile)

    start_run(
        host_list=hosts,
        mode=args.mode,
        max_power=args.max_power,
        iterations=args.iterations,
        send_buffer_size=args.send_buffer_size,
        receive_buffer_size=args.receive_buffer_size,
        warmup_iterations=args.warmup_iterations,
        message_size=args.message_size,
        ru_buffer_bytes=args.ru_buffer_bytes,
        bu_buffer_bytes=args.bu_buffer_bytes,
        logging_interval=args.logging_interval,
        explanation=args.explanation,
        non_blocking=args.non_blocking
    )

    signal.signal(signal.SIGINT, signal_handler)
    print("Press Ctrl+C to exit.")
    signal.pause()


if __name__ == "__main__":
    main()
