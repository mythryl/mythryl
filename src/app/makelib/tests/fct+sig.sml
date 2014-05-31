api Ss {
    type t
    my x:  t
}

package s :> Ss = pkg
    local
	type u = real
	my y:  u = 1.0
    in
	type t = Int
	x = 1
    end
end
