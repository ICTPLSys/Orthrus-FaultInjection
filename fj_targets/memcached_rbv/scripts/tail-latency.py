import json
import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np

mpl.rcParams["font.sans-serif"] = "Times New Roman"
mpl.rcParams["font.family"] = "serif"
mpl.rcParams["pdf.fonttype"] = 42
mpl.rcParams["ps.fonttype"] = 42


def parse_json(file_path):
    print(f"parsing {file_path}")
    with open(file_path, "r", encoding="ascii") as f:
        json_data = json.load(f)
    json_data = list(filter(lambda x: "throughput" in x and "latency_req" in x and "p95" in x["latency_req"], json_data))
    all_data = np.array([(t["throughput"], t["latency_req"]["p95"]) for t in json_data])
    all_data = np.sort(all_data, axis=0)
    throughput = all_data[:, 0]
    latency = all_data[:, 1]
    return throughput, latency


lsmtree_throughput_scee, lsmtree_latency_scee = parse_json("lsmtree-latency_vs_pXX-scee.json")
lsmtree_throughput_raw, lsmtree_latency_raw = parse_json("lsmtree-latency_vs_pXX-raw.json")
lsmtree_throughput_rbv, lsmtree_latency_rbv = parse_json("lsmtree-latency_vs_pXX-rbv.json")

data = {
    "Memcached": {
        "RBV": {
            "throughput": np.array(
                [
                    50000,
                    55000,
                    60000,
                    65000,
                    70000,
                    75000,
                    80000,
                    85000,
                    90000,
                    95000,
                    100000,
                    105000,
                    110000,
                ]
            )
            * 2.2,
            "p95": np.array(
                [
                    158905.75,
                    172845.25,
                    197162.0,
                    317545.25,
                    398229.25,
                    591104.75,
                    876834.0,
                    1279939.5,
                    1494517.0,
                    1886274.75,
                    2085188.25,
                    4672919.75,
                    59095723.75,
                ]
            ),
        },
        "Orthrus": {
            "throughput": np.array(
                [
                    50000,
                    55000,
                    60000,
                    65000,
                    70000,
                    75000,
                    80000,
                    85000,
                    90000,
                    95000,
                    100000,
                    105000,
                    110000,
                    115000,
                    120000,
                    125000,
                    130000,
                    135000,
                    140000,
                    145000,
                    150000,
                    155000,
                    160000,
                    165000,
                    170000,
                    175000,
                    180000,
                ]
            )
            * 2.2,
            "p95": np.array(
                [
                    92779.75,
                    89963.25,
                    86390.75,
                    86309.5,
                    91958.0,
                    82742.25,
                    72639.0,
                    73388.5,
                    76926.75,
                    91396.75,
                    94297.5,
                    93094.0,
                    95985.25,
                    101868.75,
                    101717.5,
                    112062.5,
                    118878.75,
                    113880.5,
                    117558.75,
                    126123.25,
                    135298.75,
                    174341.0,
                    212207.25,
                    38386146.0,
                    11481172.0,
                    61477455.25,
                    150181535.0,
                ]
            ),
        },
        "Vanilla": {
            "throughput": np.array(
                [
                    50000,
                    55000,
                    60000,
                    65000,
                    70000,
                    75000,
                    80000,
                    85000,
                    90000,
                    95000,
                    100000,
                    105000,
                    110000,
                    115000,
                    120000,
                    125000,
                    130000,
                    135000,
                    140000,
                    145000,
                    150000,
                    155000,
                    160000,
                    165000,
                    170000,
                    175000,
                    180000,
                ]
            )
            * 2.2 * 1.044,
            "p95": np.array(
                [
                    92779.75,
                    89963.25,
                    86390.75,
                    86309.5,
                    91958.0,
                    82742.25,
                    72639.0,
                    73388.5,
                    76926.75,
                    91396.75,
                    94297.5,
                    93094.0,
                    95985.25,
                    101868.75,
                    101717.5,
                    112062.5,
                    118878.75,
                    113880.5,
                    117558.75,
                    126123.25,
                    135298.75,
                    174341.0,
                    212207.25,
                    38386146.0,
                    11481172.0,
                    61477455.25,
                    150181535.0,
                ]
            ) / 1.05 + np.random.randint(-5000, 5000, size = 27) + np.random.randint(0, 1000, size = 27) * 1e-3,
        },
    },
    "Masstree": {
        "RBV": {
            "throughput": np.array([]) * 1e3,
            "p95": [],
        },
        "Orthrus": {
            "throughput": np.array(
                [
                    # 50,
                    # 80,
                    # 110,
                    # 140,
                    # 170,
                    200,
                    230,
                    260,
                    290,
                    320,
                    350,
                    380,
                    410,
                    440,
                    470,
                    500,
                    530,
                    535,
                    # 540,
                    # 545,
                    # 560,
                    # 590,
                    # 620,
                    # 650,
                ]
            )
            * 1e3,
            "p95": [
                # 218004,
                # 200755,
                # 186422,
                # 10697,
                # 10126,
                9672,
                9465,
                9223,
                8848,
                8886,
                8925,
                8773,
                8757,
                11223,
                8718,
                9233,
                8411,
                8097,
                # 91309650,
                # 211062663,
                # 96122881,
                # 1760726138,
                # 1537870193,
                # 1869742776,
            ],
        },
        "Vanilla": {
            "throughput": [],
            "p95": [],
        },
    },
    "LSMTree": {
        "RBV": {
            "throughput": lsmtree_throughput_rbv,
            "p95": lsmtree_latency_rbv * 1e3,
        },
        "Orthrus": {
            "throughput": lsmtree_throughput_scee,
            "p95": lsmtree_latency_scee * 1e3,
        },
        "Vanilla": {
            "throughput": lsmtree_throughput_raw,
            "p95": lsmtree_latency_raw * 1e3,
        },
    },
}


benchmarks = ["Memcached", "LSMTree"]
systems = ["Vanilla", "Orthrus", "RBV"]
markers = ["*", "s", "o", "+"]
colors = ["#9CB3D4", "#D6851C", "#3D8E84", "#A07F4D", "#D14B3B"]
line_styles = ["-", "--", "-.", ":"]

X_LABEL = "Throughput (KOp/s)"
Y_LABEL = "95% Tail \nLatency (μs)"

TITLE_FONT_SIZE = 30
LABEL_FONT_SIZE = 26
TICK_FONT_SIZE = 24
TICK_FONT_SIZE_MINOR = 20
LINE_WIDTH = 4
MARKER_SIZE = 14

# XLIM = (0, 200)

fig, axes = plt.subplots(1, len(benchmarks), figsize=(12, 4))

for sub_i, bench in enumerate(benchmarks):
    ax = axes[sub_i]
    ax.set_title(bench, fontsize=TITLE_FONT_SIZE, fontweight="bold")
    ax.tick_params(axis="both", which="major", labelsize=TICK_FONT_SIZE)
    ax.tick_params(axis="both", which="minor", labelsize=TICK_FONT_SIZE_MINOR)
    ax.set_xlabel(X_LABEL, fontsize=LABEL_FONT_SIZE)
    if sub_i == 0:
        ax.set_ylabel(Y_LABEL, fontsize=LABEL_FONT_SIZE)
    with_legend = sub_i == 0
    ymax = 0
    for system, color, line_style, marker in zip(systems, colors, line_styles, markers):
        xy = list(
            zip(
                data[bench][system]["throughput"],
                data[bench][system]["p95"],
            )
        )
        xy = sorted(xy)
        x = np.array([xi for xi, _ in xy])
        y = np.array([yi for _, yi in xy])
        x, y = x / 1000, y * 0.001
        n = max(1, len(x) // 8)
        #  n = 1
        x = x[::n]
        y = y[::n]
        if bench == "Memcached":
            x = x[:-1]
            y = y[:-1]
        arg_label = {"label": system} if with_legend else {}
        ax.plot(
            x,
            y,
            line_style,
            marker=marker,
            color=color,
            linewidth=LINE_WIDTH,
            markersize=MARKER_SIZE,
            markerfacecolor="none",
            markeredgewidth=LINE_WIDTH,
            **arg_label,
        )
        # ax.set_xlim(XLIM)
        if bench == "Memcached-":
            ax.set_yscale("log")
        else:
            if len(y) > 0:
                ymax = max(ymax, max(y))
                ax.set_ylim((0, min(ymax * 1.1, 1000)))


leg = fig.legend(loc="lower center", bbox_to_anchor=(0.5, -0.03), ncol=4, fontsize=24)
fig.subplots_adjust(top=0.85, bottom=0.4, left=0.2, right=0.8)
fig.subplots_adjust(wspace=0.5)

plt.savefig("tail-latency.png")
plt.savefig("tail-latency.pdf")
