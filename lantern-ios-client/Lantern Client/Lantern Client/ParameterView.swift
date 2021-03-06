//
//  ParameterView.swift
//  Lantern Client
//
//  Created by Eric Miller on 2/9/17.
//  Copyright © 2017 patternleaf LLC. All rights reserved.
//

import Foundation
import UIKit
import RxSwift
import RxCocoa

class ParameterView: UIView {
	var parameter: EffectParameter
	var nameLabel: UILabel
	
	var disposeBag: DisposeBag
	
	init(parameter: EffectParameter) {
		self.parameter = parameter
		nameLabel = UILabel()
		disposeBag = DisposeBag()
		
		super.init(frame: CGRect.zero)
		
		translatesAutoresizingMaskIntoConstraints = false
		
		backgroundColor = Style.Color.shadow
//		backgroundColor = UIColor(colorLiteralRed: 0.2, green: 0.2, blue: 0.4, alpha: 0.5)
		
		addSubview(nameLabel)
		
		heightAnchor.constraint(equalToConstant: requiredHeight).isActive = true
		
		nameLabel.translatesAutoresizingMaskIntoConstraints = false
		
		nameLabel.widthAnchor.constraint(equalTo: widthAnchor).isActive = true
		nameLabel.heightAnchor.constraint(equalToConstant: 80).isActive = true
		nameLabel.topAnchor.constraint(equalTo: topAnchor, constant: 8).isActive = true
		nameLabel.leadingAnchor.constraint(equalTo: leadingAnchor, constant: 8).isActive = true
		
		nameLabel.font = Style.Font.parameterLabel
		nameLabel.textColor = Style.Color.light
		
		parameter.name.asDriver().asObservable().subscribe(onNext: { value in
			self.nameLabel.text = value
		}).addDisposableTo(disposeBag)
	}
	
	required init?(coder aDecoder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
	
	var requiredHeight: CGFloat {
		return 50
	}
	
	override func didMoveToSuperview() {
		
	}
}
