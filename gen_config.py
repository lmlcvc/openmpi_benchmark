import json
import argparse
import numpy as np
import re
import collections


Host_descr = collections.namedtuple("Host_descr", ["name", "device", "idx"])


def host_descr_lt(lo, ro):
    return (lo.device < ro.device) and (lo.name == ro.name)


Host_descr.__lt__ = host_descr_lt

hca_conversion_map = {
    'HCA-1': 0,
    'HCA-2': 1,
    'mlx5_0': 0,
    'mlx5_1': 1,
}

dev_name_conversion = ['mlx5_0', 'mlx5_1']


def consecutive(data, stepsize=1):
    return np.split(data, np.where(np.diff(data) != stepsize)[0] + 1)

def parse_sequence(sec):
    ret_val = sum(
        ((list(range(
            *[int(b) + c
              for c, b in enumerate(a.split('-'))])) if '-' in a else [int(a)])
         for a in sec.split(',')), [])
    return ret_val

def parse_line(line):
    fields = line.split()
    if (len(fields) == 2) and (fields[1] == "DUMMY"):
        # DUMMY node
        ret_val = ("-1", -1)
    elif (len(fields) == 3):
        ret_val = (fields[1].lower(), hca_conversion_map[fields[2]])
    else:
        ret_val = ("-2", -2)

    return ret_val


def parse_file_hosts_devices(file_name, hosts_devices, pinned_hosts_devices=None):
    ret_val = []
    with open(file_name, 'r') as input_file:
        for line in input_file.readlines():
            host, device = parse_line(line)
            try:
                idx = hosts_devices.index((host, device))
                ret_val.append(Host_descr(host, device, idx))
            except ValueError:
                if (pinned_hosts_devices and ((host, device) in pinned_hosts_devices)):
                    ret_val.append(Host_descr(host, device, -2))
                else:
                    ret_val.append(Host_descr("invalid", -1, -1))

    return ret_val


def parse_file_hosts(file_name, hosts):
    reg_expr = [re.compile(host) for host in hosts]

    ret_val = []
    with open(file_name, 'r') as input_file:
        for line in input_file.readlines():
            if (any(
                    [expr.search(line.split()[1]) != None for expr in reg_expr])):
                host, device = parse_line(line)

                ret_val.append(Host_descr(host, device, len(ret_val)))
            else:
                ret_val.append(Host_descr("invalid", -1, -1))

    return ret_val


def parse_file_lines(file_name, lines):
    unique_lines = np.unique(np.array(lines))

    ret_val = []
    with open(file_name, 'r') as input_file:
        for line_num, line in enumerate(input_file.readlines()):
            if (line_num in unique_lines):
                host, device = parse_line(line)
                if(device >= 0):
                    ret_val.append(Host_descr(host, device, len(ret_val)))
                else:
                    ret_val.append(Host_descr("invalid", -1, -1))
            else:
                pass

    return ret_val

def generate_host_json(hostname, dev, rank_id):
    return [{
        'hostname': hostname,
        'ibdev': dev_name_conversion[dev],
        'rankid': rank_id,
        'utgid': f'EB_{hostname.upper()}_RU_{dev}'
    }, {
        'hostname': hostname,
        'ibdev': dev_name_conversion[dev],
        'rankid': rank_id + 1,
        'utgid': f'EB_{hostname.upper()}_BU_{dev}'
    }]


def generate_json(host_list):
    ret_val = {"hosts": []}
    rankid = 0
    sorted_hosts = sorted(host_list)

    for host in [host for host in sorted_hosts if host.idx >= 0]:
        ret_val['hosts'].extend(generate_host_json(
            host.name, host.device, rankid))
        rankid += 2

    return ret_val


def generate_hostfile(host_list):
    return [
        f'{host.name} {dev_name_conversion[host.device]}' if host.name != "-1"
        else f'{host.name}'
        for host in host_list if host.idx >= 0
    ]


def generate_shift(host_list, switch_radix):
    shift_pattern = np.array([host.idx for host in host_list])

    shift_pattern = shift_pattern.reshape(
        (len(shift_pattern) // switch_radix, switch_radix))

    mask = shift_pattern == np.full_like(shift_pattern, -1)
    shift_pattern = np.delete(shift_pattern,
                              np.nonzero(np.all(mask, axis=1)),
                              axis=0)

    mask = shift_pattern == np.full_like(shift_pattern, -1)
    shift_pattern = np.delete(shift_pattern,
                              np.nonzero(np.all(mask, axis=0)),
                              axis=1)

    # pinned nodes are identified with -2 but the EB supports only -1 for DUMMY nodes
    shift_pattern[shift_pattern == -2] = -1

    return shift_pattern

def verify_devices(hosts):
    dummy_count = sum(1 for host in hosts if host.name != "-1")
    if(dummy_count % 2 != 0):
        print("Invalid dummy node count")
        return False

    host_devices = {}
    for host in hosts:
        if host.name != "-1":
            if host.name not in host_devices:
                host_devices[host.name] = set()
            host_devices[host.name].add(host.device)

    failed_hosts = []
    for host, devices in host_devices.items():
        if len(devices) != 2 or not all(device in {0, 1} for device in devices):
            failed_hosts.append(host)

    if len(failed_hosts) != 0:
        print("Following hosts do not use exactly 2 different devices:")
        for host in failed_hosts:
            print(host)
        return False

    return True

def main():
    parser = argparse.ArgumentParser(
        description='EB benchmarking configuration script',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument("-f",
                        "--fat_tree_file",
                        help='Optimal shift pattern generated by opensm.',
                        type=str,
                        required=True)

    parser.add_argument("-j",
                        "--json",
                        help='Ouput json config file name.',
                        type=str,
                        default='config.json',
                        required=False)

    parser.add_argument("-H",
                        "--hostfile",
                        help='Ouput host file.',
                        type=str,
                        default='host.txt',
                        required=False)
    
    parser.add_argument("-l",
                        "--lines",
                        help='List of line ranges e.g. 0,1,2,3-10.',
                        type=str,
                        default='',
                        required=False)

    parser.add_argument(
        "-n",
        "--nodes",
        help="List of regular expressions used to include nodes e.g. 'tdeb0[0-9],uaeb.*'",
        type=str,
        default='.*',
        required=False)

    parser.add_argument(
        "-r",
        "--radix",
        help='switch radix (number of ports connected to the nodes).',
        type=int,
        default=4,
        required=False)
    
    args = parser.parse_args()

    if (args.lines != ''):
        lines = parse_sequence(args.lines)
        print(args.lines)
        hosts = parse_file_lines(args.fat_tree_file, lines)
    else:
        host_match = args.nodes.split(',')
        hosts = parse_file_hosts(args.fat_tree_file, host_match)

    if(verify_devices(hosts)):
        with open(args.json, 'w') as outfile:
            json.dump(generate_json(hosts), outfile, indent=2)

        with open(args.hostfile, 'w') as outfile:
            outfile.write('\n'.join(generate_hostfile(hosts)))

        shift_pattern = np.concatenate(generate_shift(hosts, args.radix))
        n_nodes = np.count_nonzero(shift_pattern != -1)
        n_dummy = len(shift_pattern) - n_nodes
        print('EB_transport.shift_pattern = {', ','.join(map(str, shift_pattern)),
            '};')
        print(
            f'// n_nodes {n_nodes} n_dummy {n_dummy} active fraction {n_nodes/(n_nodes+n_dummy)}')
        # FIXME: doesn't print dummies
    

if __name__ == "__main__":
    main()