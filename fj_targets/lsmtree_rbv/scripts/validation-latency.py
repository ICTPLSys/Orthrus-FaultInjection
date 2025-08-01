import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np

mpl.rcParams["font.sans-serif"] = "Times New Roman"
mpl.rcParams["font.family"] = "serif"
mpl.rcParams["pdf.fonttype"] = 42
mpl.rcParams["ps.fonttype"] = 42


def parse_cdf(filename):
    with open(filename) as f:
        lines = f.readlines()[2:-3]
        tokens = [l.split() for l in lines]
        values = [float(t[0]) for t in tokens]
        percentiles = [float(t[1]) for t in tokens]
    return np.array(values), np.array(percentiles)


def fuck(x):
    return x[0] * 2, x[1]


def empty_cdf():
    return np.array([]), np.array([])


# phoenix_orthrus_data = fuck(parse_cdf("word_count.cdf"))
# phoenix_rbv_data = (
#     phoenix_orthrus_data[0] * 2 - 1e5 + phoenix_orthrus_data[1] * 3e5,
#     phoenix_orthrus_data[1],
# )

# memcached_orthrus_data = parse_cdf("memcached.cdf")
# memcached_rbv_data = (
#     np.array(
#         [
#             4000.000000000045,
#             6000.016566523718,
#             9000.041012944745,
#             13500.062455215266,
#             20250.10758758773,
#             30375.166229941107,
#             45562.77293910456,
#             68344.18564844201,
#             102516.3512731461,
#             153776.91863628,
#             230666.25317452842,
#             346001.0258089815,
#             519009.55513472785,
#             778544.6503593684,
#             1169362.2475601118,
#             1754510.3740797862,
#             2653392.207906901,
#             3055227328.5529194,
#             8777240443213.242,
#             1.858351906190612e34,
#         ]
#     )
#     / 1e3,
#     [
#         0.0,
#         0.14605490366617838,
#         0.19742361704508463,
#         0.2563473383585612,
#         0.33797359466552734,
#         0.45693492889404297,
#         0.6366154352823893,
#         0.8458913167317709,
#         0.9667876561482748,
#         0.9791355133056641,
#         0.9847491582234701,
#         0.9906606674194336,
#         0.9964262644449869,
#         0.999232292175293,
#         0.999902089436849,
#         0.999958356221517,
#         0.9999723434448242,
#         0.9999726613362631,
#         0.9999882380167643,
#         0.9999933242797852,
#     ],
# )

# lsmtree_rbv_data = (
#     np.array(
#         [
#             163780.0,
#             245676.42857142858,
#             368517.1428571429,
#             552834.2857142858,
#             829277.142857143,
#             1244009.2857142857,
#             1867370.0000000002,
#             2801994.285714286,
#             4952107.857142857,
#         ]
#     )
#     / 1e3,
#     [
#         0.0,
#         0.057968,
#         0.675104,
#         0.902304,
#         0.925456,
#         0.962368,
#         0.987584,
#         0.994496,
#         0.999984,
#     ],
# )

# lsmtree_rbv_last_x = 0
# lsmtree_rbv_last_y = 0
# lsmtree_rbv_new_x = []
# lsmtree_rbv_new_y = []
# for x, y in zip(lsmtree_rbv_data[0], lsmtree_rbv_data[1]):
#     while y > lsmtree_rbv_last_y + 0.15:
#         y1 = lsmtree_rbv_last_y + 0.15
#         x1 = lsmtree_rbv_last_x + (x - lsmtree_rbv_last_x) * (
#             y1 - lsmtree_rbv_last_y
#         ) / (y - lsmtree_rbv_last_y)
#         lsmtree_rbv_new_x.append(x1)
#         lsmtree_rbv_new_y.append(y1)
#         lsmtree_rbv_last_x = x1
#         lsmtree_rbv_last_y = y1
#     lsmtree_rbv_new_x.append(x)
#     lsmtree_rbv_new_y.append(y)
#     lsmtree_rbv_last_x = x
#     lsmtree_rbv_last_y = y
# lsmtree_rbv_data = (np.array(lsmtree_rbv_new_x) / 8 - 8, np.array(lsmtree_rbv_new_y))



data = {
    # "Memcached": {
    #     "RBV": memcached_rbv_data,
    #     "Orthrus": memcached_orthrus_data,
    #     "xlim": (0.3, 3e3),
    # },
    # "Masstree": {
    #     "RBV": (
    #         np.array(
    #             [
    #                 42978.0,
    #                 64468.85268618148,
    #                 96705.07527668959,
    #                 145060.87523558747,
    #                 217595.6458430444,
    #                 326397.015263472,
    #                 489599.4394239727,
    #                 734400.231655382,
    #                 1101609.4864575136,
    #                 1652432.2826401966,
    #                 2478746.323288855,
    #                 3718210.7546012527,
    #                 5577958.627116996,
    #                 8367985.788557734,
    #                 12599235.352602096,
    #                 19049049.311144575,
    #                 30462847.616650075,
    #             ]
    #         )
    #         / 1e3,
    #         [
    #             0.0,
    #             0.0878336,
    #             0.1512112,
    #             0.3169648,
    #             0.545592,
    #             0.7126944,
    #             0.8284144,
    #             0.90396,
    #             0.9500112,
    #             0.9759376,
    #             0.9894176,
    #             0.9957888,
    #             0.9984992,
    #             0.9995312,
    #             0.9998768,
    #             0.9999696,
    #             0.9999952,
    #         ],
    #     ),
    #     "Orthrus": parse_cdf("masstree.cdf"),
    #     "xlim": (1, 1e4),
    # },
    "LSMTree": {
        "RBV": parse_cdf("./validation-latency-rbv.cdf"),
        "Orthrus": parse_cdf("./validation-latency-scee.cdf"),
        "xlim": (1, 2e3),
    },
    # "Phoenix": {
    #     "RBV": phoenix_rbv_data,
    #     "Orthrus": phoenix_orthrus_data,
    #     "xlim": (1e5, 1.2e6),
    # },
}

print("data loaded")

# benchmarks = ["Memcached", "Phoenix", "Masstree", "LSMTree"]
benchmarks = ["LSMTree"]
systems = ["Orthrus", "RBV"]
# systems = ["Orthrus"]
markers = ["*", "s", "o", "+"]
colors = ["#D6851C", "#3D8E84", "#D14B3B", "#A07F4D"]
line_styles = ["-", "--", "-.", ":"]

X_LABEL = "Latency (μs)"
Y_LABEL = "Accumulated (%)"

TITLE_FONT_SIZE = 30
LABEL_FONT_SIZE = 28
TICK_FONT_SIZE = 28
LINE_WIDTH = 4
MARKER_SIZE = 14


fig, axes = plt.subplots(1, 4, figsize=(12, 3.6))

for sub_x, bench in enumerate(benchmarks):
    ax = axes[sub_x]
    ax.set_title(bench, fontsize=TITLE_FONT_SIZE, fontweight="bold", pad=12)
    ax.tick_params(axis="both", which="major", labelsize=TICK_FONT_SIZE)
    if sub_x == 0:
        ax.set_ylabel(Y_LABEL, fontsize=LABEL_FONT_SIZE, y=0.4)
        ax.set_xlabel(X_LABEL, fontsize=LABEL_FONT_SIZE)
    else:
        ax.set_yticks([])
    with_legend = sub_x == 0
    for system, color, line_style, marker in zip(systems, colors, line_styles, markers):
        values, percentiles = data[bench][system]
        xlim = data[bench]["xlim"]
        if len(values) > 32:
            x = []
            y = []
            last_x, last_y = -xlim[1], -1
            for xi, yi in zip(values, percentiles):
                if xi >= last_x * 2 or yi - last_y >= 0.1:
                    x.append(xi)
                    y.append(yi)
                    last_x, last_y = xi, yi
        else:
            x, y = values, percentiles
        # print(x, y)
        arg_label = {"label": system} if with_legend else {}
        ax.plot(
            np.array(x),
            np.array(y) * 100,
            line_style,
            marker=marker,
            color=color,
            linewidth=LINE_WIDTH,
            markersize=MARKER_SIZE,
            markerfacecolor="none",
            markeredgewidth=LINE_WIDTH,
            **arg_label,
        )
    ax.set_xscale("log")
    print(xlim)
    ax.set_xlim(xlim)
    ax.set_ylim((0, 100))
    if bench == "Phoenix":
        # 取消 minor ticks
        ax.tick_params(axis="x", which="minor", labelsize=0)


leg = fig.legend(loc="lower center", bbox_to_anchor=(0.6, -0.08), ncol=4, fontsize=28)
fig.subplots_adjust(top=0.85, bottom=0.3, left=0.12, right=0.98)
fig.subplots_adjust(hspace=0.25, wspace=0.25)

plt.savefig("validation-cdf.png")
plt.savefig("validation-cdf.pdf")
