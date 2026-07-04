#!/usr/bin/env perl
use strict;
use warnings;
use File::Basename qw(basename dirname);
use File::Path qw(make_path);
use Getopt::Long qw(GetOptions);

my %MODULE_CACHE;
my %MODULE_PATH_CACHE;

my %PRIMITIVE_MAP = (
	'INTEGER' => {
		c_type => 'asn1_INTEGER_t',
		desc => 'asn1_INTEGER_type_desc',
		type_enum => 'ASN1_TYPE_INTEGER',
		tag => [ 'ASN1_CLASS_UNIVERSAL', 'ASN1_Primitive', 'ASN1_TYPE_INTEGER' ],
	},
	'ENUMERATED' => {
		c_type => 'asn1_ENUMERATED_t',
		desc => 'asn1_ENUMERATED_type_desc',
		type_enum => 'ASN1_TYPE_ENUMERATED',
		tag => [ 'ASN1_CLASS_UNIVERSAL', 'ASN1_Primitive', 'ASN1_TYPE_ENUMERATED' ],
	},
	'BIT STRING' => {
		c_type => 'asn1_BIT_STRING_t',
		desc => 'asn1_BIT_STRING_type_desc',
		type_enum => 'ASN1_TYPE_BIT_STRING',
		tag => [ 'ASN1_CLASS_UNIVERSAL', 'ASN1_Primitive', 'ASN1_TYPE_BIT_STRING' ],
	},
	'OCTET STRING' => {
		c_type => 'asn1_OCTET_STRING_t',
		desc => 'asn1_OCTET_STRING_type_desc',
		type_enum => 'ASN1_TYPE_OCTET_STRING',
		tag => [ 'ASN1_CLASS_UNIVERSAL', 'ASN1_Primitive', 'ASN1_TYPE_OCTET_STRING' ],
	},
	'OBJECT IDENTIFIER' => {
		c_type => 'asn1_OBJECT_IDENTIFIER_t',
		desc => 'asn1_OBJECT_IDENTIFIER_type_desc',
		type_enum => 'ASN1_TYPE_OBJECT_IDENTIFIER',
		tag => [ 'ASN1_CLASS_UNIVERSAL', 'ASN1_Primitive', 'ASN1_TYPE_OBJECT_IDENTIFIER' ],
	},
	'BOOLEAN' => {
		c_type => 'asn1_BOOLEAN_t',
		desc => 'asn1_BOOLEAN_type_desc',
		type_enum => 'ASN1_TYPE_BOOLEAN',
		tag => [ 'ASN1_CLASS_UNIVERSAL', 'ASN1_Primitive', 'ASN1_TYPE_BOOLEAN' ],
	},
	'NULL' => {
		c_type => 'asn1_NULL_t',
		desc => 'asn1_NULL_type_desc',
		type_enum => 'ASN1_TYPE_NULL',
		tag => [ 'ASN1_CLASS_UNIVERSAL', 'ASN1_Primitive', 'ASN1_TYPE_NULL' ],
	},
	'ANY' => {
		c_type => 'asn1_ANY_t',
		desc => 'asn1_ANY_type_desc',
		type_enum => 'ASN1_TYPE_ANY',
		tag => undef,
	},
	'IA5String' => {
		c_type => 'asn1_OCTET_STRING_t',
		desc => 'asn1_OCTET_STRING_type_desc',
		type_enum => 'ASN1_TYPE_OCTET_STRING',
		tag => [ 'ASN1_CLASS_UNIVERSAL', 'ASN1_Primitive', 'ASN1_TYPE_IA5String' ],
	},
	'PrintableString' => {
		c_type => 'asn1_OCTET_STRING_t',
		desc => 'asn1_OCTET_STRING_type_desc',
		type_enum => 'ASN1_TYPE_OCTET_STRING',
		tag => [ 'ASN1_CLASS_UNIVERSAL', 'ASN1_Primitive', 'ASN1_TYPE_PrintableString' ],
	},
	'NumericString' => {
		c_type => 'asn1_OCTET_STRING_t',
		desc => 'asn1_OCTET_STRING_type_desc',
		type_enum => 'ASN1_TYPE_OCTET_STRING',
		tag => [ 'ASN1_CLASS_UNIVERSAL', 'ASN1_Primitive', 'ASN1_TYPE_NumericString' ],
	},
);

my $output_dir = 'generated-asn1';
my $force = 0;
my $help = 0;
my $debug_type = '';
my $debug_imports = 0;
GetOptions(
	'out|o=s' => \$output_dir,
	'force|f!' => \$force,
	'help|h!' => \$help,
	'debug-type=s' => \$debug_type,
	'debug-imports!' => \$debug_imports,
) or usage(1);

usage(0) if $help;
@ARGV or usage(1);

make_path($output_dir) if !-d $output_dir;

if($debug_type) {
	my ($path) = @ARGV;
	my $type_name = $debug_type;
	die "--debug-type requires: <asn-file> and --debug-type <type-name>\n" if !$path || !$type_name;
	my $module = load_module_from_path($path);
	debug_type_resolution($module, $type_name);
	exit 0;
}

if($debug_imports) {
	my ($path) = @ARGV;
	die "--debug-imports requires: <asn-file>\n" if !$path;
	my $module = load_module_from_path($path);
	debug_imports_dump($module);
	exit 0;
}

for my $asn_path (@ARGV) {
	generate_from_file($asn_path, $output_dir, $force);
}

exit 0;

sub usage {
	my ($code) = @_;
	print <<"USAGE";
Usage:
  perl tools/asn1_codegen.pl [--out DIR] file1.asn [file2.asn ...]

Generates initial `.h` and `.c` descriptor skeletons for ASN.1 modules.
Current scope:
  - aliases to primitive or referenced types
  - INTEGER/ENUMERATED named values
  - BIT STRING named bits
  - SEQUENCE / CHOICE
  - SEQUENCE OF / SET OF
  - basic IMPLICIT / EXPLICIT / APPLICATION / CONTEXT tags

Notes:
  - This is an initial generator intended for iterative correction.
  - Custom decoder hooks and project-specific bounded list layouts are not inferred yet.
  - Use `--debug-type <asn-file> <type-name>` to inspect resolution of one type.
  - Use `--debug-imports <asn-file>` to inspect parsed imports.
USAGE
	exit $code;
}

sub generate_from_file {
	my ($path, $out_dir, $force_write) = @_;
	my $module = load_module_from_path($path);
	my $base = module_output_base($path);
	my $header_path = "$out_dir/$base.h";
	my $source_path = "$out_dir/$base.c";

	if(!$force_write && (-e $header_path || -e $source_path)) {
		die "Refusing to overwrite existing output for $path; use --force\n";
	}

	write_file($header_path, render_header($module, $base));
	write_file($source_path, render_source($module, $base));
	print "generated $header_path\n";
	print "generated $source_path\n";
}

sub write_file {
	my ($path, $content) = @_;
	open my $out, '>', $path or die "write $path: $!";
	print {$out} $content;
	close $out;
}

sub parse_module {
	my ($raw, $path) = @_;
	my $text = strip_comments($raw);
	$text =~ s/\r//g;

	my ($module_name) = $text =~ /^\s*([A-Za-z][A-Za-z0-9-]*)\s*(?:\{|DEFINITIONS)/m;
	$module_name ||= basename($path);

	my %imports = parse_imports($text);
	my @defs = split_definitions($text);
	my @value_defs = split_value_assignments($raw);
	my %types;
	my @warnings;

	for my $def (@defs) {
		my ($name, $rhs) = @$def{qw(name rhs)};
		next if is_value_assignment($rhs);
		my $ast;
		eval {
			$ast = parse_type_expr(tokenize($rhs), $name);
			1;
		} or do {
			my $err = $@ || 'unknown parse error';
			push @warnings, "$name skipped: $err";
			next;
		};
		$ast->{asn_name} = $name;
		$ast->{c_base} = c_base_name($module_name, $name, $path);
		$ast->{deps} = collect_dependencies($ast);
		$ast->{path} = $path;
		$ast->{value_map} = [];
		$types{$name} = $ast;
	}
	for my $value_def (@value_defs) {
		my ($symbol, $type_name, $value) = @$value_def{qw(symbol type_name value)};
		next if !exists $types{$type_name};
		push @{ $types{$type_name}{value_map} }, {
			symbol => $symbol,
			value => $value,
			leading_comments => $value_def->{leading_comments},
			trailing_comments => $value_def->{trailing_comments},
		};
	}

	my @order = topo_sort(\%types);
	return {
		name => $module_name,
		path => $path,
		base => module_output_base($path),
		imports => \%imports,
		types => \%types,
		order => \@order,
		warnings => \@warnings,
	};
}

sub load_module_from_path {
	my ($path) = @_;
	return $MODULE_CACHE{$path} if exists $MODULE_CACHE{$path};
	open my $fh, '<', $path or die "open $path: $!";
	local $/;
	my $raw = <$fh>;
	close $fh;
	my $module = parse_module($raw, $path);
	$MODULE_CACHE{$path} = $module;
	$MODULE_PATH_CACHE{$module->{name}} ||= $path;
	return $module;
}

sub find_module_path {
	my ($module_name) = @_;
	return $MODULE_PATH_CACHE{$module_name} if exists $MODULE_PATH_CACHE{$module_name};
	my @candidates = (
		$module_name,
		module_name_variants($module_name),
	);
	my %seen;
	@candidates = grep { defined($_) && length($_) && !$seen{$_}++ } @candidates;
	for my $path (glob('asn1/**/*.asn'), glob('asn1/*.asn')) {
		next if !-f $path;
		my $base = basename($path);
		$base =~ s/\.asn$//i;
		for my $cand (@candidates) {
			if($base eq $cand) {
				$MODULE_PATH_CACHE{$module_name} = $path;
				return $path;
			}
		}
		open my $fh, '<', $path or next;
		my $first = do { local $/; <$fh> };
		close $fh;
		my $text = strip_comments($first);
		for my $cand (@candidates) {
			if($text =~ /^\s*\Q$cand\E\s*(?:\{|DEFINITIONS)/m) {
				$MODULE_PATH_CACHE{$module_name} = $path;
				return $path;
			}
		}
	}
	return undef;
}

sub module_name_variants {
	my ($name) = @_;
	my @variants;
	push @variants, $name =~ s/^MAP-([A-Z]{2})-DataTypes$/MAP-$1-DataTypes/r;
	push @variants, $name =~ s/^MAP-([A-Z]{2})-Code$/MAP-$1-Code/r;
	push @variants, $name =~ s/^MAP-([A-Z]{2})-Errors$/MAP-$1-Errors/r;
	push @variants, $name =~ s/^MAP-([A-Z]{2})-Operations$/MAP-$1-Operations/r;
	return @variants;
}

sub resolve_type_definition {
	my ($module, $type_name) = @_;
	if(exists $module->{types}{$type_name}) {
		return $module->{types}{$type_name};
	}
	for my $import_module_name (keys %{ $module->{imports} || {} }) {
		my %imported = map { $_ => 1 } @{ $module->{imports}{$import_module_name} || [] };
		next if !$imported{$type_name};
		my $path = find_module_path($import_module_name);
		next if !$path;
		my $import_module = load_module_from_path($path);
		return $import_module->{types}{$type_name} if exists $import_module->{types}{$type_name};
	}
	return undef;
}

sub resolve_effective_expr {
	my ($module, $expr, $seen) = @_;
	$seen ||= {};
	return $expr if !$expr;
	my $base = $expr->{base};
	return $expr if $expr->{tag};
	return $expr if !$base || $base->{kind} ne 'type_ref';
	my $name = $base->{type_name};
	return $expr if $seen->{$name}++;
	my $ref_type = resolve_type_definition($module, $name);
	return $expr if !$ref_type;
	my $resolved = {
		%$expr,
		base => $ref_type->{base},
		tag => $ref_type->{tag},
	};
	return resolve_effective_expr(load_module_for_type($module, $ref_type), $resolved, $seen);
}

sub load_module_for_type {
	my ($current_module, $type) = @_;
	return $current_module if exists $current_module->{types}{ $type->{asn_name} || '' };
	return load_module_from_path($type->{path});
}

sub debug_type_resolution {
	my ($module, $type_name) = @_;
	my $type = resolve_type_definition($module, $type_name);
	if(!$type) {
		print "Type not resolved: $type_name\n";
		return;
	}
	print "module: $module->{name}\n";
	print "type: $type_name\n";
	print "resolved_path: $type->{path}\n";
	print "resolved_c_base: $type->{c_base}\n";
	print "resolved_kind: " . ($type->{base}{kind} // '<none>') . "\n";
	if($type->{tag}) {
		print "resolved_tag: " . type_tag_expr($type->{tag}, $type) . "\n";
	} elsif($type->{base}{tag}) {
		print "resolved_base_tag: " . type_tag_expr($type->{base}{tag}, $type) . "\n";
	} else {
		print "resolved_expr_tag: " . expr_tag_expr(load_module_for_type($module, $type), { base => $type->{base} }) . "\n";
	}
	if($type->{base}{kind} eq 'type_ref') {
		print "alias_target: $type->{base}{type_name}\n";
	}
}

sub debug_imports_dump {
	my ($module) = @_;
	print "module: $module->{name}\n";
	for my $import_module (sort keys %{ $module->{imports} || {} }) {
		print "from $import_module:\n";
		for my $name (@{ $module->{imports}{$import_module} || [] }) {
			print "  $name\n";
		}
	}
}

sub strip_comments {
	my ($text) = @_;
	$text =~ s/--[^\n]*//g;
	return $text;
}

sub parse_imports {
	my ($text) = @_;
	my %imports;
	return %imports if $text !~ /IMPORTS(.*?);/sg;
	my $block = $1;
	my @lines = split /\n/, $block;
	my @pending_names;
	for my $line (@lines) {
		$line = trim($line);
		next if $line eq '';
		if($line =~ /^FROM\s+([A-Za-z][A-Za-z0-9\-]*)/) {
			my $module = $1;
			$imports{$module} ||= [];
			push @{ $imports{$module} }, @pending_names;
			@pending_names = ();
			next;
		}
		next if $line =~ /^\{/;
		next if $line =~ /^[a-z]/;
		$line =~ s/\{.*$//;
		my @names = grep { length } map { trim($_) } split /,/, $line;
		push @pending_names, @names;
	}
	return %imports;
}

sub split_definitions {
	my ($text) = @_;
	my @defs;
	my @lines = split /\n/, $text;
	my $i = 0;

	while($i <= $#lines) {
		my $line = $lines[$i];
		if($line =~ /^\s*([A-Za-z][A-Za-z0-9-]*)\s*::=\s*(.*)$/) {
			my $name = $1;
			my $rhs = $2;
			$i++;
			while($i <= $#lines && !is_definition_start($lines[$i])) {
				last if $lines[$i] =~ /^\s*[A-Za-z][A-Za-z0-9-]*\s+[A-Za-z][A-Za-z0-9-]*\s*::=/;
				last if $lines[$i] =~ /^\s*END\b/;
				$rhs .= "\n" . $lines[$i];
				$i++;
			}
			push @defs, { name => $name, rhs => trim($rhs) };
			next;
		}
		$i++;
	}
	return @defs;
}

sub split_value_assignments {
	my ($raw) = @_;
	my @defs;
	my @lines = split /\n/, $raw;
	for(my $i = 0; $i <= $#lines; $i++) {
		my $line = $lines[$i];
		next unless $line =~ /^\s*([A-Za-z][A-Za-z0-9-]*)\s+([A-Za-z][A-Za-z0-9-]*)\s*::=\s*(.+?)\s*$/;
		my ($symbol, $type_name, $value) = ($1, $2, $3);
		next if $value =~ /^(INTEGER|ENUMERATED|BIT STRING|OCTET STRING|OBJECT IDENTIFIER|NumericString|PrintableString|IA5String|BOOLEAN|NULL|ANY|SEQUENCE|SET|CHOICE)\b/;
		my @leading_comments;
		my $leading_owner_is_value = 0;
		for(my $j = $i - 1; $j >= 0; $j--) {
			my $prev = $lines[$j];
			last if $prev =~ /^\s*$/;
			if($prev =~ /^\s*[A-Za-z][A-Za-z0-9-]*\s+[A-Za-z][A-Za-z0-9-]*\s*::=/) {
				$leading_owner_is_value = 1;
				last;
			}
			if($prev =~ /^\s*--\s?(.*)$/) {
				unshift @leading_comments, $1;
				next;
			}
			last;
		}
		@leading_comments = () if $leading_owner_is_value;
		my @trailing_comments;
		for(my $j = $i + 1; $j <= $#lines; $j++) {
			my $next = $lines[$j];
			last if $next =~ /^\s*$/;
			if($next =~ /^\s*--\s?(.*)$/) {
				push @trailing_comments, $1;
				next;
			}
			last;
		}
		push @defs, {
			symbol => $symbol,
			type_name => $type_name,
			value => $value,
			leading_comments => \@leading_comments,
			trailing_comments => \@trailing_comments,
		};
	}
	return @defs;
}

sub is_definition_start {
	my ($line) = @_;
	return 1 if $line =~ /^\s*[A-Za-z][A-Za-z0-9-]*\s*::=/;
	return 0;
}

sub tokenize {
	my ($text) = @_;
	my @tokens;
	pos($text) = 0;
	while(pos($text) < length($text)) {
		if($text =~ /\G\s+/gc) {
			next;
		}
		if($text =~ /\G(\"[^\"]*\"|\'[^\']*\'[A-Za-z]?|\|)/gc) {
			push @tokens, $1;
			next;
		}
		if($text =~ /\G(::=|\.\.\.|\.{2}|\,|\{|\}|\[|\]|\(|\))/gc) {
			push @tokens, $1;
			next;
		}
		if($text =~ /\G([0-9]+)/gc) {
			push @tokens, $1;
			next;
		}
		if($text =~ /\G([A-Za-z][A-Za-z0-9\-]*)/gc) {
			push @tokens, $1;
			next;
		}
		my $char = substr($text, pos($text), 1);
		die "Tokenizer error near [$char] in: $text\n";
	}
	return \@tokens;
}

sub parse_type_expr {
	my ($tokens, $name) = @_;
	my $state = { tokens => $tokens, idx => 0, owner => $name };
	my $expr = parse_prefixed_type($state);
	return $expr;
}

sub parse_prefixed_type {
	my ($state) = @_;
	my $expr = {
		tag => undef,
		tag_mode => undef,
		base => undef,
	};

	if(peek($state) && peek($state) eq '[') {
		$expr->{tag} = parse_tag($state);
		if(look_is($state, 'IMPLICIT') || look_is($state, 'EXPLICIT')) {
			$expr->{tag_mode} = shift_token($state);
		}
	}
	$expr->{base} = parse_base_type($state);
	return $expr;
}

sub parse_tag {
	my ($state) = @_;
	expect($state, '[');
	my $class = 'CONTEXT';
	if(look_is($state, 'APPLICATION') || look_is($state, 'PRIVATE') || look_is($state, 'UNIVERSAL')) {
		$class = shift_token($state);
	}
	my $number = shift_token($state);
	expect($state, ']');
	return {
		class => $class,
		number => $number + 0,
	};
}

sub parse_base_type {
	my ($state) = @_;
	my $tok = shift_token($state);
	die "Unexpected end of type\n" if !defined $tok;

	if($tok eq 'SEQUENCE' || $tok eq 'SET') {
		skip_size_constraint($state);
		if(look_is($state, 'OF')) {
			shift_token($state);
			my $item = parse_prefixed_type($state);
			return {
				kind => $tok eq 'SEQUENCE' ? 'sequence_of' : 'set_of',
				item => $item,
			};
		}
		expect($state, '{');
		my $fields = parse_fields($state, $tok eq 'SEQUENCE' ? 'sequence' : 'set');
		expect($state, '}');
		return {
			kind => lc($tok),
			fields => $fields,
		};
	}
	if($tok eq 'CHOICE') {
		expect($state, '{');
		my $fields = parse_fields($state, 'choice');
		expect($state, '}');
		return {
			kind => 'choice',
			fields => $fields,
		};
	}
	if($tok eq 'INTEGER' || $tok eq 'ENUMERATED') {
		my $kind = lc($tok);
		my $values = [];
		if(look_is($state, '{')) {
			$values = parse_named_values($state);
		}
		return {
			kind => $kind,
			values => $values,
		};
	}
	if($tok eq 'BIT' && look_is($state, 'STRING')) {
		shift_token($state);
		my $values = [];
		$values = parse_named_values($state) if look_is($state, '{');
		return {
			kind => 'bit_string',
			values => $values,
		};
	}
	if($tok eq 'OCTET' && look_is($state, 'STRING')) {
		shift_token($state);
		skip_constraints($state);
		return { kind => 'primitive_ref', type_name => 'OCTET STRING' };
	}
	if($tok eq 'OBJECT' && look_is($state, 'IDENTIFIER')) {
		shift_token($state);
		return { kind => 'primitive_ref', type_name => 'OBJECT IDENTIFIER' };
	}
	if($tok eq 'NumericString' || $tok eq 'PrintableString' || $tok eq 'IA5String') {
		skip_constraints($state);
		return { kind => 'primitive_ref', type_name => $tok };
	}
	if(exists $PRIMITIVE_MAP{$tok}) {
		skip_constraints($state);
		return { kind => 'primitive_ref', type_name => $tok };
	}

	my $type_name = $tok;
	while(defined(peek($state)) && peek($state) eq '-') {
		$type_name .= shift_token($state);
		$type_name .= shift_token($state);
	}
	skip_constraints($state);
	return {
		kind => 'type_ref',
		type_name => $type_name,
	};
}

sub parse_named_values {
	my ($state) = @_;
	my @values;
	expect($state, '{');
	while(defined(peek($state)) && peek($state) ne '}') {
		if(look_is($state, '...')) {
			shift_token($state);
			if(look_is($state, ',')) {
				shift_token($state);
			}
			next;
		}
		my $name = shift_token($state);
		if(look_is($state, '(')) {
			expect($state, '(');
			my $value = shift_token($state);
			expect($state, ')');
			push @values, { name => $name, value => $value };
		}
		if(look_is($state, ',')) {
			shift_token($state);
		}
	}
	expect($state, '}');
	return \@values;
}

sub parse_fields {
	my ($state, $kind) = @_;
	my @fields;
	while(defined(peek($state)) && peek($state) ne '}') {
		if(look_is($state, '...')) {
			shift_token($state);
			if(look_is($state, ',')) {
				shift_token($state);
			}
			next;
		}
		if(look_is($state, 'COMPONENTS')) {
			shift_token($state);
			expect($state, 'OF');
			my $ref_name = shift_token($state);
			push @fields, {
				name => '__components_of__' . $ref_name,
				optional => 0,
				defaulted => 0,
				components_of => 1,
				ref_name => $ref_name,
				expr => {
					tag => undef,
					tag_mode => undef,
					base => {
						kind => 'type_ref',
						type_name => $ref_name,
					},
				},
			};
			if(look_is($state, ',')) {
				shift_token($state);
			}
			next;
		}
		my $name = shift_token($state);
		next if !defined $name || $name eq ',';
		my $field = {
			name => $name,
			optional => 0,
			defaulted => 0,
		};
		my $expr = parse_prefixed_type($state);
		if(look_is($state, 'OPTIONAL')) {
			$field->{optional} = 1;
			shift_token($state);
		} elsif(look_is($state, 'DEFAULT')) {
			$field->{optional} = 1;
			$field->{defaulted} = 1;
			shift_token($state);
			skip_default_value($state);
		}
		$field->{expr} = $expr;
		push @fields, $field;
		if(look_is($state, ',')) {
			shift_token($state);
		}
	}
	return \@fields;
}

sub skip_default_value {
	my ($state) = @_;
	my $depth = 0;
	while(defined(peek($state))) {
		last if $depth == 0 && (peek($state) eq ',' || peek($state) eq '}');
		my $tok = shift_token($state);
		$depth++ if $tok eq '{' || $tok eq '(' || $tok eq '[';
		$depth-- if $tok eq '}' || $tok eq ')' || $tok eq ']';
	}
}

sub skip_constraints {
	my ($state) = @_;
	while(look_is($state, '(')) {
		my $depth = 0;
		do {
			my $tok = shift_token($state);
			$depth++ if $tok eq '(';
			$depth-- if $tok eq ')';
		} while($depth > 0 && defined(peek($state)));
	}
}

sub skip_size_constraint {
	my ($state) = @_;
	if(look_is($state, 'SIZE')) {
		shift_token($state);
		skip_constraints($state);
	}
}

sub peek {
	my ($state) = @_;
	return $state->{tokens}->[$state->{idx}];
}

sub look_is {
	my ($state, $value) = @_;
	return defined(peek($state)) && peek($state) eq $value;
}

sub shift_token {
	my ($state) = @_;
	return $state->{tokens}->[$state->{idx}++];
}

sub expect {
	my ($state, $expected) = @_;
	my $got = shift_token($state);
	die "Expected [$expected] got [" . (defined $got ? $got : 'EOF') . "]\n"
		if !defined($got) || $got ne $expected;
}

sub collect_dependencies {
	my ($type) = @_;
	my %deps;
	collect_expr_deps($type->{base}, \%deps);
	delete $deps{$type->{asn_name}};
	return [ sort keys %deps ];
}

sub collect_expr_deps {
	my ($expr, $deps) = @_;
	return if !$expr;
	my $base = $expr->{base} || $expr;
	if($base->{kind} eq 'type_ref') {
		$deps->{ $base->{type_name} } = 1 if !exists $PRIMITIVE_MAP{$base->{type_name}};
	} elsif($base->{kind} eq 'sequence' || $base->{kind} eq 'set' || $base->{kind} eq 'choice') {
		for my $field (@{ $base->{fields} || [] }) {
			collect_expr_deps($field->{expr}, $deps);
		}
	} elsif($base->{kind} eq 'sequence_of' || $base->{kind} eq 'set_of') {
		collect_expr_deps($base->{item}, $deps);
	}
}

sub topo_sort {
	my ($types) = @_;
	my %temp;
	my %perm;
	my @order;
	for my $name (sort keys %$types) {
		visit($name, $types, \%temp, \%perm, \@order);
	}
	return @order;
}

sub visit {
	my ($name, $types, $temp, $perm, $order) = @_;
	return if $perm->{$name};
	return if $temp->{$name};
	$temp->{$name} = 1;
	for my $dep (@{ $types->{$name}{deps} || [] }) {
		next if !exists $types->{$dep};
		visit($dep, $types, $temp, $perm, $order);
	}
	delete $temp->{$name};
	$perm->{$name} = 1;
	push @$order, $name;
}

sub render_header {
	my ($module, $base) = @_;
	my $guard = uc($base);
	$guard =~ s/[^A-Z0-9]/_/g;
	$guard = "__${guard}_H__";
	my $include_headers = imported_header_includes($module);
	my $body = '';

	for my $type_name (@{ $module->{order} }) {
		$body .= emit_header_type($module, $module->{types}{$type_name});
		$body .= "\n";
	}
	if(@{ $module->{warnings} || [] }) {
		$body .= "/* Skipped definitions during generation:\n";
		for my $warning (@{ $module->{warnings} }) {
			$body .= " * $warning";
			$body .= "\n";
		}
		$body .= " */\n";
	}

	return <<"HDR";
#ifndef $guard
#define $guard

#include "asn1.h"
#include <stddef.h>
#include <stdint.h>
$include_headers
$body#endif
HDR
}

sub render_source {
	my ($module, $base) = @_;
	my $header_name = "$base.h";
	my $body = '';

	for my $type_name (@{ $module->{order} }) {
		$body .= emit_source_type($module, $module->{types}{$type_name});
		$body .= "\n";
	}
	if(@{ $module->{warnings} || [] }) {
		$body .= "/* Generation warnings:\n";
		for my $warning (@{ $module->{warnings} }) {
			$body .= " * $warning";
			$body .= "\n";
		}
		$body .= " */\n";
	}

return <<"SRC";
#include "$header_name"
#include <stddef.h>
#include <stdio.h>

$body
SRC
}

sub imported_header_includes {
	my ($module) = @_;
	my @headers;
	for my $import_module (sort keys %{ $module->{imports} }) {
		push @headers, qq(#include ") . module_base_from_name($import_module) . qq(.h");
	}
	return @headers ? join("\n", @headers) . "\n" : '';
}

sub emit_header_type {
	my ($module, $type) = @_;
	my $base = $type->{base};
	my $c_type = $type->{c_base} . '_t';
	my $desc = $type->{c_base} . '_type_desc';
	my $asn_name = $type->{asn_name};

	if($base->{kind} eq 'primitive_ref') {
		my $prim = $PRIMITIVE_MAP{$base->{type_name}};
		my $extra = '';
		if(has_octet_string_value_map($type)) {
			$extra = "\nsize_t " . $type->{c_base} .
				"_json_print(const struct asn1_param *,\n\t\tconst void *ctx, char *strbuf, size_t size);";
		}
		return "/* $asn_name */\ntypedef $prim->{c_type} $c_type;\nextern const struct asn1_type_desc $desc;$extra";
	}
	if($base->{kind} eq 'type_ref') {
		my $extra = '';
		if(has_octet_string_value_map($type)) {
			$extra = "\nsize_t " . $type->{c_base} .
				"_json_print(const struct asn1_param *,\n\t\tconst void *ctx, char *strbuf, size_t size);";
		}
		return "/* $asn_name */\ntypedef " . c_type_name($module, $base->{type_name}) . " $c_type;\nextern const struct asn1_type_desc $desc;$extra";
	}
	if($base->{kind} eq 'integer' || $base->{kind} eq 'enumerated') {
		my $typedef = $PRIMITIVE_MAP{ uc($base->{kind}) }{c_type};
		my $enum = emit_named_value_enum($type, $base->{values});
		return "/* $asn_name */\n$enum" .
			"typedef $typedef $c_type;\nextern const struct asn1_type_desc $desc;";
	}
	if($base->{kind} eq 'bit_string') {
		my $bits = emit_named_value_enum($type, $base->{values});
		return "/* $asn_name */\n$bits" .
			"typedef asn1_BIT_STRING_t $c_type;\nextern const struct asn1_type_desc $desc;";
	}
	if($base->{kind} eq 'sequence_of' || $base->{kind} eq 'set_of') {
		my $item_type = field_c_type($module, $base->{item}, 0, 0);
		return <<"SEQOF";
/* $asn_name */
typedef struct $type->{c_base} {
	$item_type *items;
	uint16_t count;
} $c_type;
extern const struct asn1_type_desc $desc;
SEQOF
	}
	if($base->{kind} eq 'choice') {
		my $enum = emit_field_bit_enum($type);
		my @union;
		for my $field (@{ $base->{fields} }) {
			push @union, "\t\t" . field_c_type($module, $field->{expr}, 1, 1) . ' ' . field_member_name($field->{name}) . ';';
		}
		return "/* $asn_name */\n" .
			$enum .
			"typedef struct $type->{c_base} {\n" .
			"\tuint32_t choice;\n" .
			"\tunion {\n" .
			join("\n", @union) . "\n" .
			"\t} u;\n" .
			"} $c_type;\n" .
			"extern const struct asn1_type_desc $desc;";
	}
	if($base->{kind} eq 'sequence' || $base->{kind} eq 'set') {
		my $enum = emit_field_bit_enum($type);
		my $mask = emit_mandatory_mask($type);
		my @members = ("\tuint64_t seen_mask;");
		for my $field (@{ $base->{fields} }) {
			next if $field->{components_of};
			push @members, "\t" . field_c_type($module, $field->{expr}, $field->{optional}, 0) . ' ' . field_member_name($field->{name}) . ';';
		}
		return "/* $asn_name */\n" .
			$enum .
			$mask .
			"typedef struct $type->{c_base} {\n" .
			join("\n", @members) . "\n" .
			"} $c_type;\n" .
			"extern const struct asn1_type_desc $desc;";
	}
	die "Unhandled header kind: $base->{kind}\n";
}

sub emit_source_type {
	my ($module, $type) = @_;
	my $base = $type->{base};
	my $desc_name = $type->{c_base} . '_type_desc';
	my $ops_name = $type->{c_base} . '_type_ops';
	my $asn_name = $type->{asn_name};
	my $c_type = $type->{c_base} . '_t';
	my $tag = type_tag_expr($type->{tag} || $base->{tag}, $type);

	if($base->{kind} eq 'primitive_ref' || $base->{kind} eq 'type_ref') {
		my $src = "static const struct asn1_type_ops $ops_name = {0};\n";
		if(has_octet_string_value_map($type)) {
			$src = emit_octet_string_value_map($type) . emit_octet_string_json_print($type) .
				"static const struct asn1_type_ops $ops_name = {\n\t.json_print = " . $type->{c_base} . "_json_print,\n};\n";
		}
		$src .= "const struct asn1_type_desc $desc_name = {\n";
		$src .= "\t.name = \"$asn_name\",\n";
		$src .= "\t.type = " . descriptor_type_enum($type) . ",\n";
		$src .= "\t.sizeof_struct = sizeof($c_type),\n";
		$src .= "\t.ops = &$ops_name,\n";
		$src .= "\t.tag = $tag,\n" if defined $tag;
		$src .= "};\n";
		return "/* $asn_name */\n$src";
	}

	if($base->{kind} eq 'integer' || $base->{kind} eq 'enumerated') {
		my $enum_arr = emit_enum_string_array($type);
		my $src = $enum_arr;
		$src .= "static const struct asn1_type_ops $ops_name = {0};\n";
		$src .= "const struct asn1_type_desc $desc_name = {\n";
		$src .= "\t.name = \"$asn_name\",\n";
		$src .= "\t.type = " . descriptor_type_enum($type) . ",\n";
		$src .= "\t.sizeof_struct = sizeof($c_type),\n";
		$src .= "\t.ops = &$ops_name,\n";
		$src .= "\t.tag = $tag,\n" if defined $tag;
		if(@{ $base->{values} || [] }) {
			$src .= "\t.enum_str = " . $type->{c_base} . "_enum_str,\n";
			$src .= "\t.enum_str_sz = sizeof(" . $type->{c_base} . "_enum_str) / sizeof(void *),\n";
		}
		$src .= "};\n";
		return "/* $asn_name */\n$src";
	}

	if($base->{kind} eq 'bit_string') {
		my $bits = emit_bit_string_array($type);
		my $src = $bits;
		$src .= "static const struct asn1_type_ops $ops_name = {0};\n";
		$src .= "const struct asn1_type_desc $desc_name = {\n";
		$src .= "\t.name = \"$asn_name\",\n";
		$src .= "\t.type = ASN1_TYPE_BIT_STRING,\n";
		$src .= "\t.sizeof_struct = sizeof($c_type),\n";
		$src .= "\t.ops = &$ops_name,\n";
		$src .= "\t.tag = $tag,\n" if defined $tag;
		if(@{ $base->{values} || [] }) {
			$src .= "\t.bit_str = " . $type->{c_base} . "_bit_str,\n";
			$src .= "\t.bit_str_sz = sizeof(" . $type->{c_base} . "_bit_str) / sizeof(void *),\n";
		}
		$src .= "};\n";
		return "/* $asn_name */\n$src";
	}

	if($base->{kind} eq 'sequence_of' || $base->{kind} eq 'set_of') {
		my $params_name = $type->{c_base} . '_items';
		my $item = $base->{item};
		my $item_desc = expr_desc_name($module, $item);
		my $item_tag = expr_tag_expr($module, $item);
		my $src = <<"SEQOF";
static const struct asn1_param $params_name\[] = {
	{
		.name = "$asn_name-item",
		.tag = $item_tag,
		.type_desc = &$item_desc,
	},
	{0}
};
static const struct asn1_type_ops $ops_name = {0};
const struct asn1_type_desc $desc_name = {
	.name = "$asn_name",
	.type = @{[ $base->{kind} eq 'sequence_of' ? 'ASN1_TYPE_SEQUENCE_OF' : 'ASN1_TYPE_SEQUENCE_OF' ]},
	.sizeof_struct = sizeof($c_type),
	.ops = &$ops_name,
	.params = $params_name,
	.nb_params = SIZEOF_DESC($params_name) - 1,
@{[ defined $tag ? "\t.tag = $tag,\n" : '' ]}};
SEQOF
		return "/* $asn_name */\n$src";
	}

	if($base->{kind} eq 'choice' || $base->{kind} eq 'sequence' || $base->{kind} eq 'set') {
		my $params_name = $type->{c_base} . '_params';
		my @params;
		for my $field (@{ $base->{fields} }) {
			next if $field->{components_of};
			my $member = field_member_name($field->{name});
			my $offset = $base->{kind} eq 'choice'
				? "offsetof($c_type, u.$member)"
				: "offsetof($c_type, $member)";
			my $flags = [];
			push @$flags, 'ASN1_PARAM_OPTIONAL' if $field->{optional};
			push @$flags, 'ASN1_PARAM_IMPLICIT_TAG' if ($field->{expr}{tag_mode} || '') eq 'IMPLICIT';
			push @$flags, 'ASN1_PARAM_EXPLICIT_TAG' if ($field->{expr}{tag_mode} || '') eq 'EXPLICIT';
			my $flag_expr = @$flags ? join(' | ', @$flags) : '0';
			push @params, <<"PARAM";
	{
		.name = "$field->{name}",
		.tag = @{[ expr_tag_expr($module, $field->{expr}) ]},
		.flags = $flag_expr,
		.bit = @{[ field_bit_name($type, $field) ]},
		.offset = $offset,
		.type_desc = &@{[ expr_desc_name($module, $field->{expr}) ]},
	},
PARAM
		}
		my $mandatory = $base->{kind} eq 'choice' ? '' : "\t.mandatory_mask = " . type_mask_name($type) . ",\n";
		my $src = <<"STRUCT";
static const struct asn1_param $params_name\[] = {
@{[ join('', @params) ]}\t{0}
};
static const struct asn1_type_ops $ops_name = {0};
const struct asn1_type_desc $desc_name = {
	.name = "$asn_name",
	.type = @{[ $base->{kind} eq 'choice' ? 'ASN1_TYPE_CHOICE' : 'ASN1_TYPE_SEQUENCE' ]},
	.sizeof_struct = sizeof($c_type),
	.ops = &$ops_name,
$mandatory	.params = $params_name,
	.nb_params = SIZEOF_DESC($params_name) - 1,
@{[ defined $tag ? "\t.tag = $tag,\n" : '' ]}};
STRUCT
		return "/* $asn_name */\n$src";
	}

	die "Unhandled source kind: $base->{kind}\n";
}

sub has_octet_string_value_map {
	my ($type) = @_;
	return 0 if !@{ $type->{value_map} || [] };
	my $base = $type->{base};
	return 1 if $base->{kind} eq 'primitive_ref' && $base->{type_name} eq 'OCTET STRING';
	return 0;
}

sub emit_octet_string_value_map {
	my ($type) = @_;
	my @lines = ("static const char *" . $type->{c_base} . "_octet_str[] = {");
	for my $item (@{ $type->{value_map} || [] }) {
		my $index = parse_asn1_octet_literal($item->{value});
		next if !defined $index;
		for my $comment (@{ $item->{leading_comments} || [] }) {
			push @lines, "\t/* " . sanitize_c_comment($comment) . " */";
		}
		push @lines, "\t[$index] = \"$item->{symbol}\",";
		for my $comment (@{ $item->{trailing_comments} || [] }) {
			push @lines, "\t/* " . sanitize_c_comment($comment) . " */";
		}
	}
	push @lines, "};\n";
	return join("\n", @lines);
}

sub emit_octet_string_json_print {
	my ($type) = @_;
	my $fn = $type->{c_base} . "_json_print";
	my $table = $type->{c_base} . "_octet_str";
	my $ctype = $type->{c_base} . "_t";
	return <<"JSON";
static size_t $fn(const struct asn1_param *par,
		const void *ctx, char *strbuf, size_t size)
{
	const $ctype *value = (const $ctype *)ctx;
	uint8_t byte = value->slice.data[0];
	if(byte >= sizeof($table) / sizeof(void *) || !$table\[byte])
		return snprintf(strbuf, size, "\\\"(unknown)\\\"");
	return snprintf(strbuf, size, "\\\"%s\\\"", $table\[byte]);
}
JSON
}

sub parse_asn1_octet_literal {
	my ($value) = @_;
	return undef if $value !~ /^'([01]+)'B$/;
	my $bits = $1;
	return undef if length($bits) > 8;
	return oct("0b$bits");
}

sub sanitize_c_comment {
	my ($text) = @_;
	$text =~ s!\*/!* /!g;
	return $text;
}

sub emit_named_value_enum {
	my ($type, $values) = @_;
	return '' if !$values || !@$values;
	my @lines = ("enum {");
	for my $value (@$values) {
		push @lines, "\t" . $type->{c_base} . '_' . c_ident($value->{name}) . " = $value->{value},";
	}
	push @lines, "};\n";
	return join("\n", @lines);
}

sub emit_field_bit_enum {
	my ($type) = @_;
	my $fields = $type->{base}{fields} || [];
	my @lines = ("enum {");
	for my $field (@$fields) {
		next if $field->{components_of};
		push @lines, "\t" . field_bit_name($type, $field) . ",";
	}
	push @lines, "};\n";
	return join("\n", @lines);
}

sub emit_mandatory_mask {
	my ($type) = @_;
	my @bits;
	for my $field (@{ $type->{base}{fields} || [] }) {
		next if $field->{components_of};
		next if $field->{optional};
		push @bits, "(1ull << " . field_bit_name($type, $field) . ")";
	}
	my $expr = @bits ? join(" |\n\t ", @bits) : "0";
	return "#define " . type_mask_name($type) . " \\\n\t$expr\n";
}

sub emit_enum_string_array {
	my ($type) = @_;
	my $values = $type->{base}{values} || [];
	return '' if !@$values;
	my @lines = ("static const char *" . $type->{c_base} . "_enum_str[] = {");
	for my $value (@$values) {
		push @lines, "\t[$value->{value}] = \"$value->{name}\",";
	}
	push @lines, "};\n";
	return join("\n", @lines);
}

sub emit_bit_string_array {
	my ($type) = @_;
	my $values = $type->{base}{values} || [];
	return '' if !@$values;
	my @lines = ("static const char *" . $type->{c_base} . "_bit_str[] = {");
	for my $value (@$values) {
		push @lines, "\t[$value->{value}] = \"$value->{name}\",";
	}
	push @lines, "};\n";
	return join("\n", @lines);
}

sub descriptor_type_enum {
	my ($type) = @_;
	my $base = $type->{base};
	return 'ASN1_TYPE_INTEGER' if $base->{kind} eq 'integer';
	return 'ASN1_TYPE_ENUMERATED' if $base->{kind} eq 'enumerated';
	return 'ASN1_TYPE_BIT_STRING' if $base->{kind} eq 'bit_string';
	return 'ASN1_TYPE_SEQUENCE' if $base->{kind} eq 'sequence';
	return 'ASN1_TYPE_SET' if $base->{kind} eq 'set';
	return 'ASN1_TYPE_CHOICE' if $base->{kind} eq 'choice';
	return 'ASN1_TYPE_SEQUENCE_OF' if $base->{kind} eq 'sequence_of';
	if($base->{kind} eq 'primitive_ref') {
		return $PRIMITIVE_MAP{ $base->{type_name} }{type_enum};
	}
	if($base->{kind} eq 'type_ref') {
		return 'ASN1_TYPE_NON_PRIMITIVE';
	}
	return 'ASN1_TYPE_NON_PRIMITIVE';
}

sub type_tag_expr {
	my ($tag, $owner_type) = @_;
	return undef if !$tag;
	my %class = (
		'CONTEXT' => 'ASN1_CLASS_CONTEXT',
		'APPLICATION' => 'ASN1_CLASS_APPLICATION',
		'PRIVATE' => 'ASN1_CLASS_PRIVATE',
		'UNIVERSAL' => 'ASN1_CLASS_UNIVERSAL',
	);
	my $cls = $class{ $tag->{class} } || 'ASN1_CLASS_CONTEXT';
	my $cons = asn1_constructed_expr($owner_type, $tag->{class}, $tag->{number});
	return "ASN1_TAG_KEY($cls, $cons, $tag->{number})";
}

sub expr_tag_expr {
	my ($module, $expr) = @_;
	$expr = resolve_effective_expr($module, $expr);
	if($expr->{tag}) {
		my $tag = $expr->{tag};
		my %class = (
			'CONTEXT' => 'ASN1_CLASS_CONTEXT',
			'APPLICATION' => 'ASN1_CLASS_APPLICATION',
			'PRIVATE' => 'ASN1_CLASS_PRIVATE',
			'UNIVERSAL' => 'ASN1_CLASS_UNIVERSAL',
		);
		my $cls = $class{ $tag->{class} } || 'ASN1_CLASS_CONTEXT';
		my $cons = asn1_constructed_expr($expr);
		return "ASN1_TAG_KEY($cls, $cons, $tag->{number})";
	}
	my $base = $expr->{base};
	if($base->{kind} eq 'primitive_ref') {
		my $tag = $PRIMITIVE_MAP{ $base->{type_name} }{tag};
		return 'ASN1_TAG_KEY(ASN1_CLASS_UNIVERSAL, ASN1_Primitive, 0)' if !$tag;
		return "ASN1_TAG_KEY($tag->[0], $tag->[1], $tag->[2])";
	}
	if($base->{kind} eq 'integer') {
		return 'ASN1_TAG_KEY(ASN1_CLASS_UNIVERSAL, ASN1_Primitive, ASN1_TYPE_INTEGER)';
	}
	if($base->{kind} eq 'enumerated') {
		return 'ASN1_TAG_KEY(ASN1_CLASS_UNIVERSAL, ASN1_Primitive, ASN1_TYPE_ENUMERATED)';
	}
	if($base->{kind} eq 'bit_string') {
		return 'ASN1_TAG_KEY(ASN1_CLASS_UNIVERSAL, ASN1_Primitive, ASN1_TYPE_BIT_STRING)';
	}
	if($base->{kind} eq 'sequence') {
		return 'ASN1_TAG_KEY(ASN1_CLASS_UNIVERSAL, ASN1_Constructed, ASN1_TYPE_SEQUENCE)';
	}
	if($base->{kind} eq 'set') {
		return 'ASN1_TAG_KEY(ASN1_CLASS_UNIVERSAL, ASN1_Constructed, ASN1_TYPE_SET)';
	}
	if($base->{kind} eq 'sequence_of' || $base->{kind} eq 'set_of') {
		return 'ASN1_TAG_KEY(ASN1_CLASS_UNIVERSAL, ASN1_Constructed, ASN1_TYPE_SEQUENCE)';
	}
	if($base->{kind} eq 'choice') {
		return 'ASN1_TAG_KEY(ASN1_CLASS_UNIVERSAL, ASN1_Constructed, ASN1_TYPE_CHOICE)';
	}
	if($base->{kind} eq 'type_ref') {
		my $type_name = $base->{type_name};
		my $ref_type = resolve_type_definition($module, $type_name);
		if($ref_type) {
			my $ref_tag = $ref_type->{tag} || $ref_type->{base}{tag};
			return type_tag_expr($ref_tag, $ref_type) if $ref_tag;
			my $owner = load_module_for_type($module, $ref_type);
			return expr_tag_expr($owner, { base => $ref_type->{base} });
		}
	}
	return 'ASN1_TAG_KEY(ASN1_CLASS_UNIVERSAL, ASN1_Primitive, 0)';
}

sub asn1_constructed_expr {
	my ($expr, $class, $number) = @_;
	if(defined $class && $class eq 'UNIVERSAL' && defined $number) {
		return 'ASN1_Constructed' if $number == 16 || $number == 17;
		return 'ASN1_Primitive';
	}
	return 'ASN1_Primitive' if !$expr;
	my $base = $expr->{base};
	return 'ASN1_Constructed'
		if $base->{kind} eq 'sequence' ||
		   $base->{kind} eq 'set' ||
		   $base->{kind} eq 'sequence_of' ||
		   $base->{kind} eq 'set_of' ||
		   $base->{kind} eq 'choice';
	return 'ASN1_Primitive';
}

sub expr_desc_name {
	my ($module, $expr) = @_;
	my $base = $expr->{base};
	if($base->{kind} eq 'primitive_ref') {
		return $PRIMITIVE_MAP{ $base->{type_name} }{desc};
	}
	if($base->{kind} eq 'type_ref') {
		my $ref_type = resolve_type_definition($module, $base->{type_name});
		if($ref_type) {
			return $ref_type->{c_base} . '_type_desc';
		}
		return c_base_name($module->{name}, $base->{type_name}, $module->{path}) . '_type_desc';
	}
	if($base->{kind} eq 'sequence_of' || $base->{kind} eq 'set_of' ||
	   $base->{kind} eq 'sequence' || $base->{kind} eq 'set' ||
	   $base->{kind} eq 'choice' || $base->{kind} eq 'integer' ||
	   $base->{kind} eq 'enumerated' || $base->{kind} eq 'bit_string') {
		return 'asn1_ANY_type_desc';
	}
	die "Unknown expr kind [$base->{kind}]";
}

sub field_c_type {
	my ($module, $expr, $optional, $force_pointer) = @_;
	my $base = $expr->{base};
	my $type;
	if($base->{kind} eq 'primitive_ref') {
		$type = $PRIMITIVE_MAP{ $base->{type_name} }{c_type};
	} elsif($base->{kind} eq 'type_ref') {
		my $ref_type = resolve_type_definition($module, $base->{type_name});
		$type = $ref_type ? ($ref_type->{c_base} . '_t') : c_type_name($module, $base->{type_name});
	} elsif($base->{kind} eq 'sequence_of' || $base->{kind} eq 'set_of') {
		$type = 'asn1_ANY_t';
	} else {
		$type = 'asn1_ANY_t';
	}
	return $type . ' *' if $force_pointer;
	return $type . ' *' if $optional;
	return $type;
}

sub field_member_name {
	my ($name) = @_;
	return c_ident($name);
}

sub field_bit_name {
	my ($type, $field) = @_;
	return $type->{c_base} . '_' . c_ident($field->{name});
}

sub type_mask_name {
	my ($type) = @_;
	return $type->{c_base} . '_MANDATORY_MASK';
}

sub c_ident {
	my ($name) = @_;
	$name =~ s/([a-z0-9])([A-Z])/$1_$2/g;
	$name =~ s/-/_/g;
	$name =~ s/[^A-Za-z0-9_]/_/g;
	$name =~ s/__+/_/g;
	return lc $name;
}

sub c_base_name {
	my ($module_name, $asn_name, $path) = @_;
	my $prefix = $path =~ m{/tcap/} ? 'tcap' : 'map';
	return $prefix . '_' . c_ident($asn_name);
}

sub c_type_name {
	my ($module, $asn_name) = @_;
	my $module_name = ref($module) ? $module->{name} : $module;
	my $path = ref($module) ? $module->{path} : '';
	return c_base_name($module_name, $asn_name, $path) . '_t';
}

sub module_output_base {
	my ($path) = @_;
	my $base = basename($path);
	$base =~ s/\.asn$//i;
	$base =~ s/([a-z0-9])([A-Z])/$1-$2/g;
	$base =~ s/[^A-Za-z0-9\-]+/-/g;
	$base =~ s/^-+|-+$//g;
	return lc $base;
}

sub module_base_from_name {
	my ($name) = @_;
	$name =~ s/([a-z0-9])([A-Z])/$1-$2/g;
	$name =~ s/[^A-Za-z0-9\-]+/-/g;
	$name =~ s/^-+|-+$//g;
	return lc $name;
}

sub trim {
	my ($s) = @_;
	$s =~ s/^\s+//;
	$s =~ s/\s+$//;
	return $s;
}

sub is_value_assignment {
	my ($rhs) = @_;
	return 1 if $rhs =~ /^\d+\s*$/;
	return 1 if $rhs =~ /^\{\s*[A-Za-z0-9\-\s(),]+\}\s*$/s;
	return 0 if $rhs =~ /^(?:INTEGER|ENUMERATED|BIT STRING|OCTET STRING|OBJECT IDENTIFIER|NumericString|PrintableString|IA5String|BOOLEAN|NULL|ANY|SEQUENCE|SET|CHOICE)\b/s;
	return 0;
}
