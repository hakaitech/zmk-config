#!/usr/bin/env ruby
# frozen_string_literal: true

# Generate the 15x9 OLED layer runes used in screen_art.h.
#
# The left OLED is mounted 90deg CCW, so a rune is authored as a 3-wide x 5-tall
# block letter (what you see on the keyboard) and stored as a 5-wide x 3-tall block
# grid rotated 90deg CW. Each block is 3x3 px, so the bitmap is 15 px wide x 9 px
# high, LVGL I1 format: 8-byte palette (black, white) + 2 bytes per pixel row.
#
# Usage:
#   ruby scripts/rune.rb check          # re-derive B, S, N from screen_art.h (proves the transform)
#   ruby scripts/rune.rb emit NAME ...  # print C for the named runes defined below

LETTERS = {
  "b" => %w[110 101 110 101 110],
  "s" => %w[011 100 010 001 110],
  "n" => %w[101 111 111 101 101],
  "h" => %w[101 101 111 101 101],
  "g" => %w[011 100 101 101 011],
  "o" => %w[111 101 101 101 111],
  "t" => %w[111 010 010 010 010],
  "p" => %w[110 101 110 100 100],
}.freeze

PALETTE = [0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff].freeze

# viewer grid (5 rows x 3 cols) -> native block grid (3 rows x 5 cols), then pixels.
def bytes_for(rows)
  native = Array.new(3) { |r| Array.new(5) { |c| rows[4 - c][r] == "1" } }
  data = []
  native.each do |blk_row|
    bits = blk_row.flat_map { |on| [on ? 1 : 0] * 3 }          # 15 px
    word = bits.inject(0) { |acc, b| (acc << 1) | b } << 1      # pad to 16 bits
    3.times { data << (word >> 8) << (word & 0xff) }
  end
  PALETTE + data
end

def c_array(name, bytes)
  body = bytes.map { |b| format("0x%02x", b) }.each_slice(12).map { |s| "    #{s.join(', ')}," }
  <<~C
    static const uint8_t ltr_#{name}_map[] = {
    #{body.join("\n")}
    };
    static const lv_img_dsc_t ltr_#{name} = {
        .header.cf = LV_COLOR_FORMAT_I1,
        .header.w = 15,
        .header.h = 9,
        .data_size = sizeof(ltr_#{name}_map),
        .data = ltr_#{name}_map,
    };
  C
end

case ARGV.shift
when "check"
  src = File.read(File.join(__dir__, "..", "config/boards/shields/corne/src/screen_art.h"))
  ok = %w[b s n].all? do |k|
    m = src[/ltr_#{k}_map\[\] = \{(.*?)\};/m, 1] or abort("ltr_#{k} not found in screen_art.h")
    have = m.scan(/0x([0-9a-f]{2})/).flatten.map { |h| h.to_i(16) }
    same = have == bytes_for(LETTERS[k])
    puts "#{k}: #{same ? 'matches' : 'DIFFERS'}"
    same
  end
  exit(ok ? 0 : 1)
when "emit"
  ARGV.each { |k| puts c_array(k, bytes_for(LETTERS.fetch(k))) }
else
  abort "usage: rune.rb check | emit NAME..."
end
